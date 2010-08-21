/*
   Copyright (c) 2007-2008 Caltech. All rights reserved.
   Coded by: Joseph Schaeffer (schaeffer@dna.caltech.edu)
*/
 
#include "scomplexlist.h"
#include "options.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

//extern Options *GlobalOptions;

/*

    SComplexListEntry Constructor/Destructor


*/

SComplexListEntry::SComplexListEntry( StrandComplex *newComplex, int newid )
{
  thisComplex = newComplex;
  energy = 0.0;
  rate = 0.0;
  ee_energy.dH = 0;
  ee_energy.nTdS = 0;
  next = NULL;
  id = newid;
}


SComplexListEntry::~SComplexListEntry( void )
{
  delete thisComplex;
  if( next != NULL )
    delete next;
}

/*

    SComplexListEntry - InitializeComplex and FillData 


*/

void SComplexListEntry::initializeComplex( void )
{
  thisComplex->generateLoops();
  thisComplex->moveDisplay();
}

void SComplexListEntry::fillData( EnergyModel *em )
{
  energy = thisComplex->getEnergy() + (em->getVolumeEnergy()+em->getAssocEnergy()) * (thisComplex->getStrandCount() - 1);
  rate = thisComplex->getTotalFlux();
  // thisComplex->fillVisData( &visiblebases )
}


/*
    SComplexListEntry::printComplex
*/

void SComplexListEntry::printComplex( int printtype, EnergyModel *em )
{
  char *temp,*temp2;
  printf("Complex %02d: %s\n", id, thisComplex->getStrandNames() );
  printf("          : %s\n",thisComplex->getSequence());
  printf("          : %s\n",thisComplex->getStructure());
  if( printtype == 1)
    printf("          : Energy: (%6.6f) TotalFlux: %6.2f\n", energy - (em->getVolumeEnergy() * (thisComplex->getStrandCount() - 1)), rate);
  else if ( printtype == 2)
    printf("          : Energy: (%6.6f) TotalFlux: %6.2f\n", energy - (em->getVolumeEnergy() + em->getAssocEnergy()) * (thisComplex->getStrandCount() -1), rate);
  else
    printf("          : Energy: (%6.6f) TotalFlux: %6.2f\n", energy, rate);

}

///////////////////////////////////////////////////////////////////////
//                                                                   //
//                       SComplexList                                //
//                                                                   //
///////////////////////////////////////////////////////////////////////



/* 
   SComplexList Constructor and Destructor

*/

SComplexList::SComplexList( EnergyModel *energyModel )
{
  numentries = 0;
  first = NULL;
  dnaEnergyModel = energyModel;
  joinRate = 0.0;
  idcounter = 0;
}


SComplexList::~SComplexList( void )
{
  if( first != NULL )
    delete first;
}


/* 
   SComplexList::addComplex( StrandComplex *newComplex );
*/

SComplexListEntry *SComplexList::addComplex( StrandComplex *newComplex )
{
  if( first == NULL )
    first = new SComplexListEntry( newComplex, idcounter );
  else
    {
      SComplexListEntry *temp = new SComplexListEntry( newComplex, idcounter );
      temp->next = first;
      first = temp;
    }
  numentries++;
  idcounter++;

  return first;
}

/*
    SComplexList::initializeList
*/ 

void SComplexList::initializeList( void )
{
  SComplexListEntry *temp = first;
  while( temp != NULL )
    {
      temp->initializeComplex();
      temp->fillData( dnaEnergyModel );
      temp = temp->next;
    }
}

/*
    SComplexList::getTotalFlux
*/

double SComplexList::getTotalFlux( void )
{
  double total = 0.0;
  SComplexListEntry *temp = first;
  while( temp!= NULL )
    {
      total += temp->rate;
      temp = temp->next;
    }
  joinRate = getJoinFlux();
  total += joinRate;
  return total;
}

/*
    double SComplexList::getJoinFlux( void )

    Computes the total flux of moves which join pairs of complexes. 

    Algorithm: 
    1. Sum all exterior bases in all complexes. 
    2. For each complex, subtract that complex's exterior bases from the sum.
       2a. Using the new sum, compute sum.A * complex.T + sum.T * complex.A * sum.G * complex.C + sum.C * complex.G
       2b. Add this amount * rate per join move to total.    
*/

double SComplexList::getJoinFlux( void )
{
  SComplexListEntry *temp = first;
  struct exterior_bases *ext_bases = NULL, total_bases;

  int total_move_count = 0;
  
  total_bases.A = total_bases.G = total_bases.T = total_bases.C = 0;
  
  if( numentries <= 1 )
    return 0.0;
  

  while( temp != NULL )
    {
      ext_bases = temp->thisComplex->getExteriorBases();

      total_bases.A += ext_bases->A;
      total_bases.C += ext_bases->C;
      total_bases.G += ext_bases->G;
      total_bases.T += ext_bases->T;

      temp = temp->next;
    }

  temp = first;
  while( temp != NULL )
    {
      ext_bases = temp->thisComplex->getExteriorBases();

      total_bases.A -= ext_bases->A;
      total_bases.C -= ext_bases->C;
      total_bases.G -= ext_bases->G;
      total_bases.T -= ext_bases->T;

      total_move_count += total_bases.A * ext_bases->T;
      total_move_count += total_bases.T * ext_bases->A;
      total_move_count += total_bases.G * ext_bases->C;
      total_move_count += total_bases.C * ext_bases->G;

      temp = temp->next;
    }

  //  printf("Total join moves: %d\n",total_move_count);
  if( total_move_count == 0) 
    return -1.0;
  else
    return (double) total_move_count * dnaEnergyModel->getJoinRate();
}



/*
    SComplexList::printComplexList
*/

void SComplexList::printComplexList( int printoptions )
{
  SComplexListEntry *temp = first;

  while( temp != NULL )
    {
      temp->printComplex( printoptions, dnaEnergyModel );
      temp = temp->next;
    }
}

/*
    SComplexList::doBasicChoice( double choice, double newtime )
*/

void SComplexList::doBasicChoice( double choice, double newtime )
{
  double rchoice = choice, moverate;
  int type;
  SComplexListEntry *temp,*temp2 = first;
  StrandComplex *pickedComplex= NULL,*newComplex = NULL;
  Move *tempmove;
  char *struc;

  if( rchoice < joinRate )
    {
      doJoinChoice( rchoice );
      return;
    }
  else
    {
      rchoice -= joinRate;
    }

  temp = first;
  while( temp != NULL )
    {
      if( rchoice < temp->rate && pickedComplex == NULL)
	{
	  pickedComplex = temp->thisComplex;
	  temp2 = temp;
	}
      if( pickedComplex == NULL )
	{
	  //	  assert( rchoice != temp->rate && temp->next == NULL);
	  rchoice -= temp->rate;
	
	}
      temp = temp->next;
    }

  assert( pickedComplex != NULL );

  tempmove = pickedComplex->getChoice( &rchoice );
  moverate = tempmove->getRate();
  type = tempmove->getType();
  newComplex = pickedComplex->doChoice( tempmove );
  if( newComplex != NULL )
    {
      temp = addComplex(newComplex);
      temp->fillData( dnaEnergyModel );
    }

  //  if( GlobalOptions->getOutputState() )
  // printf("Move done: Complex %d, Move rate: %6.4f Type: %d New time: %6.2f\n",temp2->id, moverate,type,newtime);

  temp2->fillData( dnaEnergyModel );

  /*  struc = pickedComplex->getStructure();
  int temp1=0;
  for( int loop = 0; loop < strlen(struc); loop++)
    {
      if(struc[loop] == '(') temp1++;
      if(struc[loop] == ')') temp1--;
      assert( temp1 >= 0 );
      if(loop > 0)
	assert( !(struc[loop-1]=='(' && struc[loop] == ')'));
    }
  
    assert( temp1 == 0 );*/

}


/*
    SComplexList::doJoinChoice( double choice )
*/



void SComplexList::doJoinChoice( double choice )
{
  SComplexListEntry *temp = first, *temp2 =  NULL;
  struct exterior_bases *ext_bases = NULL,*ext_bases_temp=NULL, total_bases;
  StrandComplex *deleted;
  StrandComplex *picked[2] = {NULL,NULL};
  char types[2] = {0,0};
  int index[2] = {0,0};
  int int_choice;
  int total_move_count = 0;
  
  int_choice = (int) floor(choice / dnaEnergyModel->getJoinRate());
  total_bases.A = total_bases.G = total_bases.T = total_bases.C = 0;
  
  if( numentries <= 1 )
    return;
  

  while( temp != NULL )
    {
      ext_bases = temp->thisComplex->getExteriorBases();

      total_bases.A += ext_bases->A;
      total_bases.C += ext_bases->C;
      total_bases.G += ext_bases->G;
      total_bases.T += ext_bases->T;

      temp = temp->next;
    }

  temp = first;
  while( temp != NULL )
    {
      ext_bases = temp->thisComplex->getExteriorBases();

      total_bases.A -= ext_bases->A;
      total_bases.C -= ext_bases->C;
      total_bases.G -= ext_bases->G;
      total_bases.T -= ext_bases->T;

      if( int_choice < total_bases.A * ext_bases->T )
	{
	  picked[0] = temp->thisComplex;
	  types[0] = 4;
	  types[1] = 1;
	  temp = temp->next;
	  while( temp != NULL )
	    {
	      ext_bases_temp = temp->thisComplex->getExteriorBases();

	      if( int_choice < ext_bases_temp->A * ext_bases->T )
		{
		  picked[1] = temp->thisComplex;
		  index[0] = (int) floor( int_choice / ext_bases_temp->A );
		  index[1] = int_choice - index[0]*ext_bases_temp->A;
		  temp = NULL;
		}
	      else
		{
		  temp = temp->next;
		  int_choice -= ext_bases_temp->A * ext_bases->T;
		}
	    }
	  continue; // We must have picked something, thus temp must be NULL and we need to exit the loop.
	}
      else
	int_choice -= total_bases.A * ext_bases->T;

      if( int_choice < total_bases.T * ext_bases->A )
	{
	  picked[0] = temp->thisComplex;
	  types[0] = 1;
	  types[1] = 4;
	  temp = temp->next;
	  while( temp != NULL )
	    {
	      ext_bases_temp = temp->thisComplex->getExteriorBases();

	      if( int_choice < ext_bases_temp->T * ext_bases->A )
		{
		  picked[1] = temp->thisComplex;
		  index[0] = (int) floor( int_choice / ext_bases_temp->T );
		  index[1] = int_choice - index[0]*ext_bases_temp->T;
		  temp = NULL;
		}
	      else
		{
		  temp = temp->next;
		  int_choice -= ext_bases_temp->T * ext_bases->A;
		}
	    }
	  continue;
	}
      else
	int_choice -= total_bases.T * ext_bases->A;

      if( int_choice < total_bases.G * ext_bases->C )
	{
	  picked[0] = temp->thisComplex;
	  types[0] = 2;
	  types[1] = 3;
	  temp = temp->next;
	  while( temp != NULL )
	    {
	      ext_bases_temp = temp->thisComplex->getExteriorBases();

	      if( int_choice < ext_bases_temp->G * ext_bases->C )
		{
		  picked[1] = temp->thisComplex;
		  index[0] = (int) floor( int_choice / ext_bases_temp->G);
		  index[1] = int_choice - index[0]*ext_bases_temp->G;
		  temp = NULL;
		}
	      else
		{
		  temp = temp->next;
		  int_choice -= ext_bases_temp->G * ext_bases->C;
		}
	    }
	  continue;
	}
      else
	int_choice -= total_bases.G * ext_bases->C;

      if( int_choice < total_bases.C * ext_bases->G )
	{
	  picked[0] = temp->thisComplex;
	  types[0] = 3;
	  types[1] = 2;
	  temp = temp->next;
	  while( temp != NULL )
	    {
	      ext_bases_temp = temp->thisComplex->getExteriorBases();

	      if( int_choice < ext_bases_temp->C * ext_bases->G )
		{
		  picked[1] = temp->thisComplex;
		  index[0] = (int) floor( int_choice / ext_bases_temp->C );
		  index[1] = int_choice - index[0]*ext_bases_temp->C;
		  temp = NULL;
		}
	      else
		{
		  temp = temp->next;
		  int_choice -= ext_bases_temp->C * ext_bases->G;
		}
	    }
	  continue;
	}
      else
	int_choice -= total_bases.C * ext_bases->G;

      if( temp != NULL )
	temp = temp->next;
    }

  deleted = StrandComplex::performComplexJoin( picked, types, index );

  //  if( GlobalOptions->getOutputState() && !(GlobalOptions->getSimulationMode() & SIMULATION_PYTHON))
  // printf("Join Chosen: %d x %d (%d,%d)\n",index[0],index[1], types[0],  types[1]);

  for( temp = first; temp != NULL; temp = temp->next )
    {
      if( temp->thisComplex == picked[0] )
	temp->fillData( dnaEnergyModel );
      if( temp->next != NULL )
	if( temp->next->thisComplex == deleted )
	  {
	    temp2 = temp->next;
	    temp->next = temp2->next;
	    temp2->next = NULL;
	    delete temp2;
	  }

    }
  if( first->thisComplex == deleted )
    {
      temp2 = first;
      first = first->next;
      temp2->next = NULL;
      delete temp2;

    }
  numentries--;

  return;
}

/*

   int SComplexList::checkStopComplexList( class complex_item *stoplist )

*/
int SComplexList::checkStopComplexList( class complex_item *stoplist )
{
  //if( stoplist->type == STOPTYPE_STRUCTURE || stoplist->type == STOPTYPE_DISASSOC )
  if( stoplist->type == STOPTYPE_BOUND )
    return checkStopComplexList_Bound( stoplist );
  else // type = STOPTYPE_STRUCTURE, STOPTYPE_DISASSOC, STOPTYPE_LOOSE or STOPTYPE_COUNT_OR_PERCENT
    return checkStopComplexList_Structure_Disassoc( stoplist );  
}

/*

   int SComplexList::checkStopComplexList_Bound( class complex_item *stoplist )

*/

int SComplexList::checkStopComplexList_Bound( class complex_item *stoplist )
{
  class identlist *id_traverse = stoplist->strand_ids;
  class SComplexListEntry *entry_traverse = first;
  int k_flag;
  if( stoplist->next != NULL ) 
    {
      fprintf(stderr,"ERROR: (scomplexlist.cc) Attempting to check for multiple complexes being bound, not currently supported.\n");
      return 0;  // ERROR: can only check for a single complex/group being bound in current version.
    }

  // Check for each listed strand ID, and whether it's bound. 
  // Note that we iterate through each complex in the list until we 
  // find one where it's bound, and then move on to the next.
  //
  while( id_traverse != NULL )
    {
      entry_traverse = first;
      k_flag = 0;
      while( entry_traverse != NULL && k_flag == 0)
	{
	  k_flag += entry_traverse->thisComplex->checkIDBound( id_traverse->id );
	  entry_traverse = entry_traverse->next;
	}
      if( k_flag == 0 )
	return 0;
      id_traverse = id_traverse->next;
    }

  return 1;
}

/*

   int SComplexList::checkStopComplexList_Structure_Disassoc( class complex_item *stoplist )

*/


int SComplexList::checkStopComplexList_Structure_Disassoc( class complex_item *stoplist )
{
  class SComplexListEntry *entry_traverse = first;
  class complex_item *traverse = stoplist;
  int id_count=0,max_complexes=0; 
  int successflag;
  class identlist *id_traverse = stoplist->strand_ids;


  //  if( traverse->type == STOPTYPE_BOUND )


  while( traverse != NULL )
    {
      max_complexes++;
      traverse = traverse->next;
    }
  if( max_complexes > numentries ) return 0; // can't match more entries than we have complexes.

  traverse = stoplist;  
  while( traverse != NULL )
    {
      // We are checking each entry in the list of stop complexes, verifying that it exists within our list of complexes. So the outer iteration is over the stop complexes, and the inner iteration is over the complexes existant in our system.
      // If we reach the end of the iteration successfully, we have every stop complex represented by at least one complex within the system.
      // WARNING:: this intentionally allows a single complex within the system to satisfy two (or more) stop complexes. We need to think further about such complicated stop conditions and what they might represent.
      // WARNING (cont): One exception is we can't match a list of stop complexes that has more complexes than the system currently does. This is somewhat arbitrary and could be removed.

      // count how many strands are in the stop complex. This gets used as a fast check later.
      id_count = 0;
      id_traverse = traverse->strand_ids;
	  
      while( id_traverse != NULL )
	{
	  id_count++;
	  id_traverse = id_traverse->next;
	}

      entry_traverse = first;
      successflag = 0;
      while( entry_traverse != NULL && successflag == 0)
	{
	  // iterate check for current stop complex (traverse) in our list of system complexes (entry_traverse)
	  if( entry_traverse->thisComplex->checkIDList( traverse->strand_ids, id_count ) > 0 )
	    {
	      // if the system complex being checked has the correct circular permutation of strand ids, continue with our checks, otherwise it doesn't match.
	      if( traverse->type == STOPTYPE_STRUCTURE )
		{
		  if( strcmp(entry_traverse->thisComplex->getStructure(), traverse->structure) == 0 )
		    {
		      // if the structures match exactly, we have a successful match.
		      successflag = 1;
		    }
		}
	      else if (traverse->type == STOPTYPE_DISASSOC )
		{
		  // for DISASSOC type checking, we only need the strand id lists to match correctly.
		  successflag = 1;
		}
	      else if( traverse->type == STOPTYPE_LOOSE_STRUCTURE )
		{
		  if( checkLooseStructure( entry_traverse->thisComplex->getStructure(), traverse->structure) == 0 )
		    {
		      // the structure matches loosely (see definitions)
		      successflag = 1;
		    }
		}
	      else if( traverse->type == STOPTYPE_PERCENT_OR_COUNT_STRUCTURE)
		{
		  if( checkCountStructure( entry_traverse->thisComplex->getStructure(), traverse->structure, traverse->count ) == 0)
		    {
		      // this structure matches to within a % of the correct base pairs, note that %'s are converted to raw base counts by the IO system.
		      successflag = 1;
		    }
		}
	    }
	  entry_traverse = entry_traverse->next;
	}
      if( successflag == 0 )
	return 0;
      // we did not find a successful match for this stop complex in any system complex.
      
      // otherwise, we did, try checking the next stop complex.
      traverse = traverse->next;
    }
	      
  return 1;
}

/* 
   Methods used for checking loose structure definitions and counting structure defs.

  int SComplexList::checkLooseStructure( char *our_struc, char *stop_struc );
  int SComplexList::checkCountStructure( char *our_struc, char *stop_struc, int count );

*/


int SComplexList::checkLooseStructure( char *our_struc, char *stop_struc )
{
  int loop,len;
  int *pairqueue;
  int index[2] = {0,0};
  // WARNING: implicit assumption that strings are NULL terminated

  len = strlen(our_struc);
  pairqueue = new int [len+2];

  for( loop = 0; loop <= len; loop ++ )
    {
      if( our_struc[loop] == '\0' && stop_struc[loop] == '\0' )
	{
	  delete[] pairqueue;
	  return 0;
	}
      if( our_struc[loop] == '\0' || stop_struc[loop] == '\0' )
	{
	  delete[] pairqueue;
	  return 1;
	}


      if( stop_struc[loop] != '*' )
	if( our_struc[loop] != stop_struc[loop] )
	  {
	    delete[] pairqueue;
	    return 1;
	  }

      if( our_struc[loop] == '(' && stop_struc[loop] == '(' )
	{
	  pairqueue[2*index[0]] = loop;
	  pairqueue[2*index[1]+1] = loop;
	  index[0]++;
	  index[1]++;
	}
      else if( our_struc[loop] == '(' )
	{
	  pairqueue[2*index[0]] = loop;
	  index[0]++;
	}

      if( our_struc[loop] == ')' && stop_struc[loop] == ')' )
	{
	  index[0]--;
	  index[1]--;
	  assert( (index[0] >= 0) && (index[1] >= 0) );

	  if( pairqueue[2*index[0]] != pairqueue[2*index[1]+1] )
	    {
	      delete [] pairqueue;
	      return 1;
	    }
	}
      else if( our_struc[loop] == ')' )
	index[0]--;
    }
}


int SComplexList::checkCountStructure( char *our_struc, char *stop_struc, int count )
{
  int loop=0;
  int ncount=0;
  while(!(our_struc[loop] == '\0' && stop_struc[loop] == '\0' ))
    {
      if( our_struc[loop] == '\0' || stop_struc[loop] == '\0' )
	return 1;
      
      if( stop_struc[loop] != '*' )
	if( our_struc[loop] != stop_struc[loop] )
	  {
	    if( stop_struc[loop] == '_' || our_struc[loop] == '_' )
	      return 1;
	    else
	      ncount++;
	  }
      
      loop++;
    }
  if (ncount <= count)
    return 0;
  else
    return 1;
}