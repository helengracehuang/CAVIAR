#ifndef MCAVIARMODEL_H
#define MCAVIARMODEL_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <armadillo>

#include "MPostCal.h"

using namespace std;
using namespace arma;

bool not_find(vector<string> vec, string str){
    for(int i = 0; i < vec.size(); i++) {
        if (vec[i] == str)
            return true;
    }
    return false;
}

void remove(mat& LD, vector<string>*& name, vector<double>*& z_score, int pos){
    name->erase(name->begin() + pos);
    z_score->erase(z_score->begin() + pos);
    LD.shed_row(pos);
    LD.shed_col(pos);
}

void Msort(vector<int>& index, vector<string>& name, vector<double>& z_score, mat& LD){
    vector<string>* new_name = new vector<string>;
    vector<double>* new_z = new vector<double>;
    
    for(int i = 0; i < index.size(); i++){
        new_name->push_back(name.at(index.at(i)));
        new_z->push_back(z_score.at(index.at(i)));
    }
    
    name = *new_name;
    z_score = *new_z;
    
    mat* temp_LD_1 = new mat(size(LD));
    for(int i = 0; i < index.size(); i++){
        temp_LD_1->row(i) = LD.row(index[i]);
    }
    *temp_LD_1 = trans(*temp_LD_1);
    
    mat* temp_LD_2 = new mat(size(LD));
    for(int i = 0; i < index.size(); i++){
        temp_LD_2->row(i) = temp_LD_1->row(index[i]);
    }
    
    *temp_LD_2 = trans(*temp_LD_2);
    LD = *temp_LD_2;
}

vector<string>* find_intersection(vector<string>* name_1, vector<string>* name_2){
    vector<string>* inters = new vector<string>;
    sort(name_1->begin(), name_1->end());
    sort(name_2->begin(), name_2->end());
    int i = 0;
    int j = 0;
    
    do{
        if(name_1->at(i) == name_2->at(j)) {
            inters->push_back(name_1->at(i));
            i++;
            j++;
        }
        else if(name_1->at(i) < name_2->at(j)) {
            i++;
        }
        else{
            j++;
        }
    } while(i<name_1->size() && j<name_2->size());
    
    return inters;
}


class MCaviarModel{
public:
    double rho;
    double NCP;
    double gamma;
    int snpCount;
    int totalCausalSNP;
    vector<mat> * sigma;
    vector< vector<double> > * z_score;
    vector<char> * pcausalSet;
    vector<int> * rank;
    bool histFlag;
    MPostCal * post;
    vector< vector<string> > * snpNames;
    string ldFile;
    string zFile;
    string outputFileName;
    string geneMapFile;
    double tau_sqr;
    double sigma_g_squared;
    int num_of_studies;
    vector<double> S_LONG_VEC;
    
    MCaviarModel(string ldFile, string zFile, string outputFileName, int totalCausalSNP, double NCP, double rho, bool histFlag, double gamma=0.01, double tau_sqr = 0.2, double sigma_g_squared = 5.2) {
        int tmpSize = 0;
        this->histFlag = histFlag;
        this->NCP = NCP;
        this->rho = rho;
        this->gamma = gamma;
        this->ldFile = ldFile;
        this->zFile  = zFile;
        this->outputFileName = outputFileName;
        this->totalCausalSNP = totalCausalSNP;
        this->tau_sqr = tau_sqr;
        this->sigma_g_squared = sigma_g_squared;
        
        fileSize(ldFile, tmpSize);
        //snpCount   = (int)sqrt(tmpSize);
        sigma      = new vector<mat>;
        z_score       = new vector<vector<double> >;
        //pcausalSet = new vector<char>;
        //rank       = new vector<int>;
        snpNames   = new vector<vector<string> >;
        
        vector<double>* temp_LD = new vector<double>;
        vector<string>* temp_names = new vector<string>;
        vector<double>* temp_z = new vector<double>;
        
        //importData(ldFile, temp_LD);
        //importDataFirstColumn(zFile, temp_names);
        //importDataSecondColumn(zFile, temp_z);
        
        string LD = "50_LD.txt";
        string Z = "50_Z.txt";
        outputFileName = "result";
        
        importData(LD, temp_LD);
        importDataFirstColumn(Z, temp_names);
        importDataSecondColumn(Z, temp_z);
        
        mat temp_sig;
        temp_sig = mat(50,50);
        for (int i = 0; i <50; i++){
            for (int j = 0; j<50; j++){
                temp_sig(i,j) = temp_LD->at(i * 50 + j); }
        }
        
        sigma->push_back(temp_sig);
        snpNames->push_back(*temp_names);
        z_score->push_back(*temp_z);
        
        
        //ASSUMING snpNames is vector of vectors of string now, sigma is a vector of matrix now, z-score is a vector of vector of double
        vector<string> intersect = (*snpNames)[0];
        for(int i = 1 ; i < snpNames->size(); i++){
            intersect = (*find_intersection(&intersect, &((*snpNames)[i])));
        }
        
        for(int i = 0; i < snpNames->size(); i++){
            int j = (*snpNames)[i].size() - 1;
            do {
                if(not_find(intersect, (*snpNames)[i][j])) {
                    (*snpNames)[i].erase((*snpNames)[i].begin() + j);
                    (*z_score)[i].erase((*z_score)[i].begin() + j);
                    (*sigma)[i].shed_row(j);
                    (*sigma)[i].shed_col(j);
                }
                j--;
            } while(j <= 0);
        }
        
        for(int i = 0; i < snpNames->size(); i++){
            vector<string> temp_names;
            temp_names = snpNames->at(i);
            sort(temp_names.begin(), temp_names.end());
            vector<int> index;
            int j = 0;
            while(j < temp_names.size()) {
                for(int k = 0; k < (*snpNames)[i].size(); k++){
                    if((temp_names)[j] == (*snpNames)[i][k]){
                        index.push_back(k);
                        j++;
                    }
                }
           }
            Msort(index, (*snpNames)[i], (*z_score)[i], (*sigma)[i]);
        }
        
        num_of_studies = snpNames->size();
        snpCount = (*snpNames)[0].size();
        pcausalSet = new vector<char>(snpCount);
        rank = new vector<int>(snpCount, 0);
        
        for (int i = 0; i < z_score->size(); i++){
            for(int j = 0; j < (*z_score)[i].size(); j++){
                S_LONG_VEC.push_back((*z_score)[i][j]);
            }
        }
        
        for(int i = 0 ; i < num_of_studies; i++){
            for (int j = 0; j < snpCount; j++){
                if(abs(S_LONG_VEC.at(i*snpCount + j)) > NCP){
                    NCP = abs(S_LONG_VEC.at(i*snpCount + j));
                }
            }
        }
        
        for (int i = 0; i < sigma->size(); i++){
            makeSigmaPositiveSemiDefinite(&(sigma->at(i)), snpCount);
        }
        
        mat* BIG_SIGMA = new mat(snpCount * num_of_studies, snpCount * num_of_studies);
        for (int i = 0 ; i < sigma->size(); i++){
            mat temp_sigma = mat(num_of_studies,num_of_studies);
            temp_sigma(i,i) = 1;
            
            temp_sigma = kron(temp_sigma, sigma->at(i));
            (*BIG_SIGMA) = (*BIG_SIGMA) + temp_sigma;
        }
        
        post = new MPostCal(BIG_SIGMA, &S_LONG_VEC, snpCount, totalCausalSNP, snpNames, gamma, tau_sqr, sigma_g_squared, num_of_studies);
    }
    
    void run() {
        post->findOptimalSetGreedy(&S_LONG_VEC, NCP, pcausalSet, rank, rho, outputFileName);
    }
    
    void finishUp() {
        ofstream outputFile;
        string outFileNameSet = string(outputFileName)+"_set";
        outputFile.open(outFileNameSet.c_str());
        for(int i = 0; i < snpCount; i++) {
            if((*pcausalSet)[i] == '1')
                outputFile << (*snpNames)[0][i] << endl;
        }
        post->printPost2File(string(outputFileName)+"_post");
        //output sthe histogram data to file
        if(histFlag)
            post->printHist2File(string(outputFileName)+"_hist");
    }
    
    void printLogData() {
        //print likelihood
        //print all possible configuration from the p-causal set
        post->computeALLCausalSetConfiguration(&S_LONG_VEC, NCP, pcausalSet,outputFileName+".log");
    }
    
    ~MCaviarModel() {}
    
};

#endif
