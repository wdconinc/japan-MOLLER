/**********************************************************\
* File: QwFakeHelicity.C                                  *
*                                                         *
* Author: B.Waidyawansa                                   *
* Time-stamp:03-06-2010                                   *
\**********************************************************/



#include "QwFakeHelicity.h"

// Register this subsystem with the factory
RegisterSubsystemFactory(QwFakeHelicity);


 Bool_t QwFakeHelicity::IsGoodHelicity()
{
  fGoodHelicity = kTRUE;  
  return fGoodHelicity;

}

 void  QwFakeHelicity::ProcessEvent()
{

  Bool_t fake_the_counters=kFALSE; 

  // First get the information from the Input Register. Even though there are no helicity 
  // information we can still get the MPS, Pattern SYNC and Pattern phase.

  fEventNumber=fWord[kMpsCounter].fValue;
  // When we don't have the pattern information in the input register, fake them.
  if(fake_the_counters){
    if (fEventNumber == 1){  // first pattern
      fPatternNumber      = 0;
      fPatternPhaseNumber = fMinPatternPhase;
    }  
    else  if (fPatternPhaseNumber == fMaxPatternPhase){ // end of a pattern
      fPatternPhaseNumber = fMinPatternPhase;
      fPatternNumber      = fPatternNumberOld + 1;
    } 
    else  {
      fPatternPhaseNumber  = fPatternPhaseNumberOld + 1;
      fPatternNumber       = fPatternNumberOld; 
    }
  }

  // When we do have the pattern information in the inputregister, use thm.
  if( !fake_the_counters ){
      fPatternNumber      = fWord[kPatternCounter].fValue;
      fPatternPhaseNumber = fWord[kPatternPhase].fValue;
  }
  

  fHelicityActual=0;  
  fHelicityBitPlus=kTRUE;
  fHelicityBitMinus=kFALSE;
  PredictHelicity();

  return;
}

 void QwFakeHelicity::ClearEventData()
 {
   for (size_t i=0;i<fWord.size();i++)
     fWord[i].ClearEventData();
  
   //reset data
   fEventNumberOld        = fEventNumber;
   fPatternNumberOld      = fPatternNumber;
   fPatternPhaseNumberOld = fPatternPhaseNumber;

   //clear data
   fHelicityActual   = kUndefinedHelicity;
   fHelicityBitPlus  = kFALSE;
   fHelicityBitMinus = kFALSE;
   return;
 }

 UInt_t QwFakeHelicity::GetRandbit(UInt_t& ranseed){
   Bool_t status = false;
   status = GetRandbit24(ranseed);
   return status;
 }

  Bool_t QwFakeHelicity::CollectRandBits()
 {
   static Bool_t firsttimethrough = kTRUE;

   Bool_t  ldebug = kFALSE;
   UInt_t  ranseed = 0x2535D5&0xFFFFFF; //put a mask. 
  
  /**The value of the radseed was picked up from the studies 
     Buddhini did on the 24 bit helicity generater back in 2008.
  */
   // A modification to set the random seeds that are usually generated by the first 24 patterns.
   if(! firsttimethrough){
     return kTRUE;
   } else{
     firsttimethrough = kFALSE;
     fGoodHelicity = kFALSE; //reset before prediction begins
     iseed_Delayed = ranseed;
     // Go 24 patterns back to get the reported helicity at this event
     for(UInt_t i=0;i<24;i++) 
       fDelayedPatternPolarity =GetRandbit(iseed_Delayed);
     fHelicityDelayed = fDelayedPatternPolarity;
	   
     iseed_Actual = iseed_Delayed;
     for(Int_t i=0; i<8; i++)
       fActualPatternPolarity = GetRandbit(iseed_Actual);  
     fHelicityActual =  fActualPatternPolarity;
   }

   if(ldebug) std::cout<<"QwFakeHelicity::CollectRandBits24 => Done collecting ...\n";
   return kTRUE;
  
}