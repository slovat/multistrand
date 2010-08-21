/*
   Copyright (c) 2007-2008 Caltech. All rights reserved.
   Coded by: Joseph Schaeffer (schaeffer@dna.caltech.edu)
*/
 
/* Loop class header for main loop class and derivatives. */ 

#ifndef __LOOP_H__
#define __LOOP_H__

#include "energymodel.h"
#include "move.h"

class Loop
{
 public:
  inline double getEnergy( void );
  inline double getTotalRate( void );
  char getType( void );
  Loop( void );
  virtual ~Loop( void );
  virtual void calculateEnergy( void ) = 0;
  virtual void generateMoves( void ) = 0;
  virtual void generateDeleteMoves( void ) = 0;
  virtual Move *getChoice( double *randomchoice, Loop *from ) = 0;
  virtual double doChoice( Move *move, Loop **returnLoop ) = 0;
  virtual char *getLocation( Move *move, int index ) =0;
  virtual char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from ) =0;
  Loop *getAdjacent( void );
  void addAdjacent( Loop *loopToAdd );
  void initAdjacency( int index );
  int replaceAdjacent( Loop *loopToReplace, Loop *loopToReplaceWith );
  void cleanupAdjacent( void ); // sets adjacentLoops up to be deleted.
  double returnEnergies( Loop *comefrom ); // returns the total energy of all loops underneath this one.
  double returnFlux( Loop *comefrom ); // returns the total energy of all loops underneath this one.
  virtual void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p ) = 0;
  virtual void printMove( Loop *comefrom, char *structure_p, char *seq_p ) = 0;
  void firstGen( Loop *comefrom );
  static void SetEnergyModel( EnergyModel *newEnergyModel );
  static EnergyModel *GetEnergyModel( void );
  static double generateDeleteMoveRate( Loop *start, Loop *end );
  static Loop *performDeleteMove( Move *move );
  static void performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );

 protected:
  static EnergyModel *energyModel_Primary;
  Loop **adjacentLoops;
  int numAdjacent;
  int curAdjacent;
  double energy;
  int energyFlag;
  double totalRate;
  MoveContainer *moves;
  char identity;
  int add_index;
};

class StackLoop : public Loop
{
 public:
  void calculateEnergy( void);
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randnum , Loop *from);
  double doChoice( Move *move, Loop **returnLoop );
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );
  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend Loop * Loop::performDeleteMove( Move *move );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );
  StackLoop( void );
  StackLoop( int type1, int type2, char *seq1, char *seq2, Loop *left = NULL, Loop *right = NULL);
 private:
  int pairtype[2];
  char *seqs[2];
};

class HairpinLoop : public Loop
{
 public:
  void calculateEnergy( void);
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randnum, Loop *from );
  double doChoice( Move *move, Loop **returnLoop  );
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );

  HairpinLoop( void );
  HairpinLoop( int type, int size, char *hairpin_sequence, Loop *previous = NULL );
  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend Loop * Loop::performDeleteMove( Move *move );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );

 private:
  int pairtype;
  int hairpinsize;
  char *hairpin_seq;
};

class BulgeLoop : public Loop
{
 public:
  void calculateEnergy( void );
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randnum , Loop *from);
  double doChoice( Move *move, Loop **returnLoop );
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );
  BulgeLoop( void );
  ~BulgeLoop( void );
  BulgeLoop( int type1, int type2, int size1, int size2, char *bulge_sequence1, char *bulge_sequence2, Loop *left = NULL, Loop *right = NULL);
  friend Loop * Loop::performDeleteMove( Move *move );
  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );

 private:
  int pairtype[2];
  int bulgesize[2];
  char *bulge_seq[2];
};

class InteriorLoop : public Loop
{
 public:
  void calculateEnergy( void );
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randnum , Loop *from);
  double doChoice( Move *move, Loop **returnLoop );
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );
  InteriorLoop( void );
  InteriorLoop( int type1, int type2, int size1, int size2, char *int_seq1, char *int_seq2, Loop *left = NULL, Loop *right = NULL);

  friend Loop * Loop::performDeleteMove( Move *move );
  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );

 private:
  int pairtype[2];
  int sizes[2];
  char *int_seq[2];
};

class MultiLoop : public Loop
{
 public:
  void calculateEnergy( void );
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randnum , Loop *from);
  double doChoice( Move *move, Loop **returnLoop );  
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );
  MultiLoop( void );
  MultiLoop( int branches, int *pairtypes, int *sidelengths, char **sequences );
  ~MultiLoop( void );

  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend Loop * Loop::performDeleteMove( Move *move );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );

 private:
  int *pairtype;
  int *sidelen;
  char **seqs;
};

class OpenLoop : public Loop
{
 public:
  void calculateEnergy( void );
  void generateMoves( void );
  void generateDeleteMoves( void );
  Move *getChoice( double *randomchoice, Loop *from );
  double doChoice( Move *move, Loop **returnLoop );
  void moveDisplay( Loop *comefrom , char *structure_p, char *seq_p );
  void printMove( Loop *comefrom , char *structure_p, char *seq_p );
  char *getLocation( Move *move, int index );
  char *verifyLoop( char *incoming_sequence, int incoming_pairtype, Loop *from );
  //  void addAdjacent( Loop *loopToAdd );


  // OpenLoop::getFreeBases returns the base composition information for the
  //   open loop. Return form is a pointer to an array of size 5, containing
  //   frequence counts of each type of base, plus invalid/out of bounds bases.
  //   Calling function needs to free the array. 
  // TODO: Possibly change this so it returns the index of a static array inside the function, so there's no memory overhead for the function call. (Data is accessed single-threaded, and information is copied out by the calling function.) Could also make it a private data member that's just returned.  Not completed currently.
  int *getFreeBases( void );

  char *getBase( char type, int index );

  OpenLoop( void );
  OpenLoop( int branches, int *pairtypes, int *sidelengths, char **sequences );
  ~OpenLoop( void );
  friend double Loop::generateDeleteMoveRate( Loop *start, Loop *end );
  friend Loop * Loop::performDeleteMove( Move *move );
  friend void Loop::performComplexSplit( Move *move, Loop **firstOpen, Loop **secondOpen );
  static void performComplexJoin( OpenLoop **oldLoops, OpenLoop **newLoops, char *types, int *index);
 private:
  int *pairtype;
  int *sidelen;
  char **seqs;
};
#endif