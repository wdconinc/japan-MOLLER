#ifndef __CAMREG__
#define __CAMREG__
#include "../camguin.hh"
#include <vector>
#include <TString.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <TChain.h>
using namespace std;
void regress_h(TString tree = "mul", Int_t runNumber = 0, Int_t nRuns = -1, TString regInput = "regressionInput.txt", char delim = ' '){
  runNumber = getRunNumber_h(runNumber);
  nRuns     = getNruns_h(nRuns);
  vector<vector<string>> textFile = textFileParse_h(regInput,delim);
  TTree * oldTree = getTree_h(tree, runNumber, nRuns, "NULL");
  TTreeReader oldTreeReader(oldTree);
  TTree * newTree = new TTree("reg"+tree,"Regressed "+tree+" tree");
  if (oldTree==0){
    Printf("Root file does not exist");
    return;
  }

  Double_t data     = 0.0;
  Int_t    n_data   = 0;
  vector<Double_t> newRegressedValues;
  vector<Double_t> parameters;
  vector<Double_t> weighting;
  vector<Double_t> oldManipulatedValues;
  vector<Double_t> oldManipulatedErrors;
  vector<Double_t> oldRespondingValues;
  vector<Double_t> oldRespondingErrors;
  vector<TString> newRegressedBranchList;
  vector<TString> oldManipulatedDataBranchList;
  vector<TString> oldManipulatedErrorBranchList;
  vector<TString> oldRespondingDataBranchList;
  vector<TString> oldRespondingErrorBranchList;
  TString errorBranchName  = "ErrorFlag";
  Double_t oldTreeErrorFlag = 0.0;
  Bool_t resp  = false;
  Int_t nresp  = 0;
  Bool_t manip = false;
  Int_t nmanip = 0;
  if (debug > -1) Printf("Data structures initialized");
  for (UInt_t listEntryN = 0; listEntryN < textFile.size(); listEntryN++){
    if (textFile[listEntryN][0] == "Global"){
      if (debug > 2) Printf("Branch %s",textFile[listEntryN][1].c_str());
      errorBranchName = textFile[listEntryN][1];
      resp = false;
      manip = false;
      continue;
    }
    if (textFile[listEntryN][0] == "Responding"){
      resp = true;
      manip = false;
      continue;
    }
    if (textFile[listEntryN][0] == "Manipulated"){
      manip = true;
      resp = false;
      continue;
    }
    if (resp){
      if (debug > 2) Printf("Branch %s",textFile[listEntryN][0].c_str());
      oldRespondingDataBranchList.push_back(textFile[listEntryN][0]+textFile[listEntryN][1]);
      oldRespondingValues.push_back(0.0); 
      oldRespondingErrorBranchList.push_back(textFile[listEntryN][0]+textFile[listEntryN][2]);
      oldRespondingErrors.push_back(0.0); 
      newRegressedBranchList.push_back("reg_"+textFile[listEntryN][0]);
      newRegressedValues.push_back(0.0); 
      nresp++;
    }
    if (manip){
      if (debug > 2) Printf("Branch %s",textFile[listEntryN][0].c_str());
      oldManipulatedDataBranchList.push_back(textFile[listEntryN][0]+textFile[listEntryN][1]);
      oldManipulatedValues.push_back(0.0); 
      oldManipulatedErrorBranchList.push_back(textFile[listEntryN][0]+textFile[listEntryN][2]);
      oldManipulatedErrors.push_back(0.0); 
      parameters.push_back(stof(textFile[listEntryN][3])); // Initial parameter correlation slope guess for iterating fit
      weighting.push_back(stof(textFile[listEntryN][4])); // Initial weighting guess for errors
      nmanip++;
    }
    if (debug > 1) Printf("Branch %d initialized",listEntryN);
  }
  oldManipulatedValues.push_back(1.0); // The physics asymmetry is asymmetry*1 (the number 1 is the type of value it is, an exact scalar)
  oldManipulatedErrors.push_back(0.0); // The physics asymmetry applies for all Global Cuts passing entries
  parameters.push_back(0.0); // Push back the physics asymmetry value placeholder at nmanip+1 position... assume it is trivially 0 for first pass
  weighting.push_back(1.0); // Push back the physics asymmetry relative weighting factor for uncertainty calculations
  nmanip++; // The physics asymmetry value adds ++ to the parameters of the fit
  Int_t nmanipInputs = nmanip-1;

  vector<TTreeReaderValue<Double_t>> oldRespondingValuesReader;
  vector<TTreeReaderValue<Double_t>> oldRespondingErrorsReader;
  vector<TTreeReaderValue<Double_t>> oldManipulatedValuesReader;
  vector<TTreeReaderValue<Double_t>> oldManipulatedErrorsReader;

  for(Int_t iBranch = 0; iBranch < oldManipulatedDataBranchList.size(); iBranch++) {
    TTreeReaderValue<Double_t> temp1(oldTreeReader,oldManipulatedDataBranchList[iBranch]);
    TTreeReaderValue<Double_t> temp2(oldTreeReader,oldManipulatedErrorBranchList[iBranch]);
    oldManipulatedValuesReader.push_back(temp1);
    oldManipulatedErrorsReader.push_back(temp2);
    //oldTree->SetBranchAddress(oldManipulatedDataBranchList[iBranch],&oldManipulatedValues[iBranch]);
    //oldTree->SetBranchAddress(oldManipulatedErrorBranchList[iBranch],&oldManipulatedErrors[iBranch]);
  }
  oldManipulatedDataBranchList.push_back("PV_Asymmetry");
  for(Int_t iBranch = 0; iBranch < oldRespondingDataBranchList.size(); iBranch++) {
    TTreeReaderValue<Double_t> temp1(oldTreeReader,oldRespondingDataBranchList[iBranch]);
    TTreeReaderValue<Double_t> temp2(oldTreeReader,oldRespondingErrorBranchList[iBranch]);
    oldRespondingValuesReader.push_back(temp1);
    oldRespondingErrorsReader.push_back(temp2);
    //oldTree->SetBranchAddress(oldRespondingDataBranchList[iBranch],&oldRespondingValues[iBranch]);
    //oldTree->SetBranchAddress(oldRespondingErrorBranchList[iBranch],&oldRespondingErrors[iBranch]);
    newTree->Branch(          newRegressedBranchList[iBranch],&newRegressedValues[iBranch]);
  }
  oldTree->SetBranchAddress(errorBranchName,&oldTreeErrorFlag);
  newTree->SetBranchStatus("*",1);
  oldTree->SetBranchStatus("*",1);
  if (debug > -1) Printf("Branches initialized");

  Double_t rate = 1.0e9;
  Double_t integrationFreq = 240.0;
  Double_t uncertainty = 1.0/sqrt(rate/integrationFreq);

  //FIXME make these into branches that get stored in output tree instead of large vectors here
  vector<Double_t> f; // The functional value integrated over all data entries
  vector<Double_t> s; // The sigma value integrated over all data entries
  vector<Double_t> chi2;
  Double_t chi2sum;
  Double_t fi = 0.0; // The functional value per data entry
  Double_t si2 = nmanip*uncertainty*uncertainty;// The total sigma value per data entry
  Double_t si = 0.0; // The total sigma value per data entry
  Double_t chi2i = 0.0;
  vector<Double_t> dfi;   // The first derivative of the functional value per data entry
  vector<Double_t> dsi;   // The first derivative of the sigma value per data entry
  vector<Double_t> dsi2;  // The first derivative of the sigma value per data entry
  vector<vector<Double_t>> ddfi;  // The second derivative of the functional value per data entry
  vector<vector<Double_t>> ddsi;  // The second derivative of the sigma value per data entry
  vector<vector<Double_t>> ddsi2; // The second derivative of the sigma value per data entry
  vector<vector<vector<Double_t>>> dddfi; // The third derivative of the functional value per data entry

  vector<Double_t> beta; // The x^2 slope vector
  vector<Double_t> betai; // The x^2 slope vector
  vector<vector<Double_t>> alpha; // The x^2 curvature matrix
  vector<vector<Double_t>> alphai; // The x^2 curvature matrix
  vector<vector<Double_t>> covariance; // The Covariance matrix = alpha^-1
  vector<vector<Double_t>> covariancei; // The Covariance matrix = alpha^-1
  vector<Double_t> delta_parameters; // The change in parameter vector = alpha^-1*beta
  vector<Double_t> placeholder;
  vector<vector<Double_t>> placeholder2;
  vector<vector<vector<Double_t>>> placeholder3;
  if (debug > -1) Printf("Empty matrices");
  for (Int_t l = 0; l<nmanip; l++){
    placeholder.push_back(0.0); // Make a vector of 0s
    dfi.push_back(0.0);
    dsi.push_back(0.0);
    dsi2.push_back(0.0);
    beta.push_back(0.0);
    betai.push_back(0.0);
    delta_parameters.push_back(0.0);
  }
  for (Int_t k = 0; k<nmanip; k++){
    placeholder2.push_back(placeholder); // Make a matrix of vectors of 0s
    ddfi.push_back(placeholder);
    ddsi.push_back(placeholder);
    ddsi2.push_back(placeholder);
    alpha.push_back(placeholder);
    alphai.push_back(placeholder);
    covariance.push_back(placeholder);
  }
  for (Int_t j = 0; j<nmanip; j++){
    dddfi.push_back(placeholder2); // Make a 3-tensor of matrices of vectors of 0s
    placeholder3.push_back(placeholder2);
  }

  Int_t fitN = 0;
  Int_t errorFlag = 0;
  Int_t numEntries = oldTree->GetEntries();
  if (debug > -1) Printf("Looping over %d entries",numEntries);
  if (numEntries>5000) numEntries=5000;
  if (debug > -1) Printf("Looping over %d entries",numEntries);
  Int_t i = 0;
  Int_t iterateAgain = 1;
  while(iterateAgain){
  //while(iterateAgain!=0)
  //while(oldTreeReader.Next() && oldTreeReader.GetCurrentEntry() > (iterateAgain-1)*numEntries && oldTreeReader.GetCurrentEntry() < iterateAgain*numEntries)
  while(oldTreeReader.Next() && oldTreeReader.GetCurrentEntry()<numEntries){
  //for (int i = 0; i < numEntries; i++)  // Loop over the input file's entries
    //oldTree->GetEntry(i);
    if (oldTreeErrorFlag==0){
      for (Int_t j = 0 ; j<nmanip ; j++){ // Loop over fit parameters j
        if (debug > 3) Printf("Looping, j = %d",j);
        if (j<nmanipInputs){
          oldManipulatedValues[j]=*oldManipulatedValuesReader[j];
          oldManipulatedErrors[j]=*oldManipulatedErrorsReader[j];
        }
        else {
          oldManipulatedValues[j]=1.0; // User input values here;
        }
        if(oldManipulatedErrors[j]==0){
          fi += parameters[j]*oldManipulatedValues[j]; // Functional form of f
          dfi[j] = 1*oldManipulatedValues[j]; // Functional form of first derivative of f
          for (Int_t k = 0 ; k<nmanip ; k++){ // Loop over fit parameters k
            ddfi[j][k] = 0; // Functional form of second derivative of f
            for (Int_t l = 0 ; l<nmanip ; l++){ // Loop over fit parameters l
              dddfi[j][k][l] = 0; // Functional form of third derivative of f
            }
          } // end l
        } // end k
        else {
          if (debug > 2) Printf("Entry %d has an error in input %s",i,(const char*)oldManipulatedDataBranchList[j]);
          errorFlag = 1;
          continue; // Abort this entry
        }
      } // end j (f)
      if((*oldRespondingErrorsReader[fitN])!=0){ 
        if (debug > 2) Printf("Entry %d has an error in input %s",i,(const char*)oldRespondingDataBranchList[fitN]);
        errorFlag = 1; 
      }
      if (errorFlag==1){ 
        errorFlag = 0;
        continue; 
      }
      for (Int_t j = 0 ; j<nmanip ; j++){ // Loop over fit parameters j
        si2 += covariance[j][j]*dfi[j]*dfi[j];
        for (Int_t k = j+1; k<nmanip ; k++){ // Loop over fit parameters k
          si2  += 2*dfi[j]*dfi[k]*covariance[j][k];
        }
        si = sqrt(si2);
        //chi2i += (oldRespondingValues[fitN]-fi)*(oldRespondingValues[fitN]-fi)/(si2);
        if (si2 == 0){
          chi2i += (*oldRespondingValuesReader[fitN]-fi)*(*oldRespondingValuesReader[fitN]-fi);
        }
        else if (si2 > 0.0){
          chi2i += (*oldRespondingValuesReader[fitN]-fi)*(*oldRespondingValuesReader[fitN]-fi)/(si2);
        }
        else {
          continue;
        }
      }
      f.push_back(fi);
      newRegressedValues[fitN] = *oldRespondingValuesReader[fitN] - fi + (parameters[nmanip]*1); // Regressed asymmetry of main detector [fitN]
      newTree->Fill();
      s.push_back(si);
      chi2.push_back(chi2i);
      chi2sum+=chi2i;
      for (Int_t j = 0 ; j<nmanip ; j++){ // Loop over fit parameters j
        for (Int_t k = 0; k<nmanip ; k++){ // Loop over fit parameters k
          dsi2[j] += 2*covariance[k][k]*dfi[k]*ddfi[k][j];
          for (Int_t l = k+1; l<nmanip ; l++){ // Loop over fit parameters l
            dsi2[j] += 2*covariance[k][l]*((ddfi[j][k]*dfi[l])+(ddfi[j][l]*dfi[k]));
          }
          dsi[j] = dsi2[j]/(2*si);
        }
      }
      for (Int_t j = 0 ; j<nmanip ; j++){ // Loop over fit parameters j
        for (Int_t k = 0; k<nmanip ; k++){ // Loop over fit parameters k
          for (Int_t l = 0; l<nmanip ; l++){ // Loop over fit parameters l
            ddsi2[j][k] += 2*covariance[l][l]*((ddfi[l][k]*dfi[j])+(ddfi[j][k]*dfi[l]));
            for (Int_t m = l+1; m<nmanip ; m++){
              ddsi2[j][k] += 2*covariance[l][m]*((dddfi[j][k][l]*dfi[m])+(ddfi[l][j]*ddfi[m][k])+(dddfi[j][m][k]*dfi[l])+(ddfi[j][m]*ddfi[l][k]));
            }
            ddsi[j][k] = ((ddsi2[j][k]/(2*si))-(dsi2[j]*dsi2[k]/(4*si*si*si)));
          }
        }
      }
      for (Int_t j = 0 ; j<nmanip ; j++){ // Loop over fit parameters j
        betai[j] += (2*(((*oldRespondingValuesReader[fitN]-fi)*(*oldRespondingValuesReader[fitN]-fi)/(si*si*si))*dsi[j])+(2*(*oldRespondingValuesReader[fitN]-fi)*dfi[j]/(si*si)));
        //betai[j] += (2*(*oldRespondingValuesReader[fitN]-fi)*dfi[j]/(si*si));
        beta[j] += betai[j];
        for (Int_t k = 0; k<nmanip ; k++){ // Loop over fit parameters k
          alphai[j][k] += 0.5*(4*(dsi[j]*(*oldRespondingValuesReader[fitN]-fi)*dfi[k]/(si*si*si))+(6*(*oldRespondingValuesReader[fitN]-fi)*(*oldRespondingValuesReader[fitN]-fi)*dsi[j]*dsi[k]/(si*si*si*si))+(-2*(*oldRespondingValuesReader[fitN]-fi)*(*oldRespondingValuesReader[fitN]-fi)*ddsi[j][k]/(si*si*si))+(4*(*oldRespondingValuesReader[fitN]-fi)*dsi[k]*dfi[j]/(si*si*si))+(2*(dfi[j]*dfi[k]/(si*si)))+(-2*(*oldRespondingValuesReader[fitN]-fi)*ddfi[j][k]/(si*si)));
          //alphai[j][k] += 0.5*(4*(dsi[j]*(*oldRespondingValuesReader[fitN]-fi)*dfi[k]/(si*si*si))+(2*(dfi[j]*dfi[k]/(si*si)))+(-2*(*oldRespondingValuesReader[fitN]-fi)*ddfi[j][k]/(si*si)));
          alpha[j][k] += alphai[j][k];
          if (debug > 4) Printf("Beta = %f, Alpha = %f",betai[j],alpha[j][k]);
        }
      }
      if (debug > 3) Printf("Functional value at event %d = %f, sigma = %f, chi2 = %f",i,fi,si,chi2i);

      // End storing values
      fill_n(dsi.begin(),nmanip,0.0);
      fill_n(dsi2.begin(),nmanip,0.0);
      fill_n(dfi.begin(),nmanip,0.0);
      fill_n(betai.begin(),nmanip,0.0);
      fill_n(alphai.begin(),nmanip,placeholder);
      fill_n(ddsi.begin(),nmanip,placeholder);
      fill_n(ddsi2.begin(),nmanip,placeholder);
      fill_n(ddfi.begin(),nmanip,placeholder);
      fill_n(dddfi.begin(),nmanip,placeholder2);
      errorFlag = 0;
      fi=0.0;
      chi2i=0.0;
      si=0.0;
      si2=nmanip*uncertainty*uncertainty;
      
      if (debug > 4) Printf("Done entry %d",i);
      i++;
    }
  }
  // invert alpha
  covariance = inverse_h(alpha,covariance);
  if (covariance[0][0]==-999999.0){
    Printf("Error, uninvertable matrix");
    return;
  }
  // multiply epsilon*beta = a vector
  for (Int_t j = 0 ; j<beta.size(); j++){
    for (Int_t k = 0 ; k<beta.size(); k++){
      delta_parameters[j]+=covariance[j][k]*beta[k];
    }
  }
  iterateAgain = 0;
  if (debug > -1) {
    Printf("Change in parameter vector from initial guess: ");
    displayVector_h(delta_parameters);
    Printf("Original set of parameters: ");
    displayVector_h(parameters);
  }
  for (Int_t j = 0 ; j<parameters.size(); j++){
    Printf("Relative change in parameter %d = %f",j,(delta_parameters[j]+parameters[j])/parameters[j]);
    if (abs(abs((parameters[j]+delta_parameters[j])/parameters[j])-1)>0.01){
      iterateAgain = 1;
      oldTreeReader.Restart();
      newTree->Reset();
      newTree->SetEntries(0);
    }
    parameters[j]=parameters[j]+0.3*delta_parameters[j];
  }
  if (debug > -1) {
    Printf("New set of parameters: ");
    displayVector_h(parameters);
    Printf("Chi2 = %f, reduced Chi2 = %f",chi2sum,chi2sum/(numEntries-nmanip));
  }
  if (iterateAgain == 0){
    if (debug > 2) {
      Printf("Alpha matrix: ");
      displayMatrix_h(alpha,-2);
      Printf("Beta vector: ");
      displayVector_h(beta);
    }
    TFile *outFile = new TFile("outputReg.root","RECREATE");
    outFile->cd();
    // Fill a histogram with yi - fi + last parameter*1 weighted by error on that
    //gROOT->SetBatch(kTRUE);
    //gROOT->SetBatch(kFALSE);
    TCanvas * c1 = new TCanvas();
    c1->SetLogy();
    newTree->Draw(Form("%s",(const char*)newRegressedBranchList[fitN]));
    TH1 *h1 = (TH1*)gROOT->FindObject("htemp");
    TH1 *h2 = rebinTH1_h(h1,"clean",1,2,1000); // example use case of rebinTH1_h method
    TString h2_name = h2->GetName();
    newTree->Draw(Form("%s>>%s",(const char*)newRegressedBranchList[fitN],(const char*)h2_name)); // Manual
    h2->Write(Form("%s_histogram",(const char*)newRegressedBranchList[fitN]));

    TCanvas * c2 = new TCanvas();
    c2->SetLogy();

    oldTree->Draw(Form("%s",(const char*)oldRespondingDataBranchList[fitN]));
    TH1 *h1old = (TH1*)gROOT->FindObject("htemp");
    TH1 *h2old = rebinTH1_h(h1old,"clean",1,2,1000); // example use case of rebinTH1_h method
    TString h2old_name = h2old->GetName();
    oldTree->Draw(Form("%s>>%s",(const char*)oldRespondingDataBranchList[fitN],(const char*)h2old_name)); // Manual
    h2old->Write(Form("%s_histogram",(const char*)oldRespondingDataBranchList[fitN]));
    if (debug > -1) {
      Printf("Covariance matrix: ");
      displayMatrix_h(covariance,-2);
      for (Int_t j = 0 ; j<parameters.size(); j++){
        Printf("Parameter %s +- error = %5.3e +- %5.3e",(const char*)oldManipulatedDataBranchList[j],parameters[j],sqrt(covariance[j][j]));
      }
      Printf("Regressed reg %s average %5.3e +- %5.3e, std dev %5.3e +- %5.3e",(const char*)oldRespondingDataBranchList[fitN],h2->GetMean(),h2->GetMeanError(),h2->GetRMS(),h2->GetRMSError());
      Printf("Regressed old %s average %5.3e +- %5.3e, std dev %5.3e +- %5.3e",(const char*)oldRespondingDataBranchList[fitN],h2old->GetMean(),h2old->GetMeanError(),h2old->GetRMS(),h2old->GetRMSError());
    }
    newTree->Write("reg"+tree);
    outFile->Close();
  }
  chi2sum = 0;

  // Reset
  chi2.clear();
  f.clear();
  s.clear();
  fill_n(beta.begin(),nmanip,0.0);
  fill_n(delta_parameters.begin(),nmanip,0.0);
  fill_n(alpha.begin(),nmanip,placeholder);

  }
  //writeFile_h("test_n_data",n_data,runNumber,nRuns);
}
#endif // __CAMREG__
