
#include <stdio.h>

#include "TBranch.h"
#include "TBufferFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"

#include "SillyStruct.h"

int main(int argc, char *argv[]) {

    TFile *hfile;
    TTree *tree;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [read|write]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "read") == 0) {
        hfile = new TFile("SillyStruct.root");
/*
        // Read via standard APIs.
        TTreeReader myReader("T", hfile);
        TTreeReaderValue<float> a(myReader, "a");
        TTreeReaderValue<int> b(myReader, "b");
        TTreeReaderValue<double> c(myReader, "c");
        TTreeReaderValue<float> myF(myReader, "myFloat");
        TTreeReaderValue<SillyStruct> ss(myReader, "myEvent");
        while (myReader.Next()) {
            std::cout << "A=" << *a << ", B=" << *b << ", C=" << *c << ", myFloat=" << *myF << "\n";
        }
*/
        tree = dynamic_cast<TTree*>(hfile->Get("T"));
        TBranch *standardBranchF = tree->GetBranch("myFloat");
        float myFloat;
        standardBranchF->SetAddress(&myFloat);
        standardBranchF->GetEntry(0);
        std::cout << "myFloat=" << myFloat << "\n";
       
        hfile->Close();
        hfile = new TFile("SillyStruct.root");

        // Read using bulk APIs.
        TBufferFile branchbuf(TBuffer::kWrite, 10000);
        TTree *tree = dynamic_cast<TTree*>(hfile->Get("T"));
        if (!tree) {
            std::cout << "Failed to fetch tree named 'T' from input file.\n";
            return 1;
        }
        TBranch *branchF = tree->GetBranch("myFloat");
        if (!branchF) {
            std::cout << "Unable to find branch 'myFloat' in tree 'T'\n";
            return 1;
        }
        auto count = branchF->GetEntriesFast(0, branchbuf);
        if (count < 0) {
            std::cout << "Failed to get entries via the 'fast' method.\n";
            return 1;
        }
        float *entry = reinterpret_cast<float*>(branchbuf.GetCurrent());
        for (Int_t idx=0; idx<count; idx++) {
            std::cout << "myFloat=" << entry[idx] << "\n";
        }
    } else if (strcmp(argv[1], "write") == 0) {
        hfile = new TFile("SillyStruct.root","RECREATE","TTree benchmark ROOT file");
        hfile->SetCompressionLevel(1);
        tree = new TTree("T", "An example ROOT tree of SillyStructs.");
        SillyStruct ss;
        TBranch *branch = tree->Branch("myEvent", &ss, 32000, 1);
        branch->SetAutoDelete(kFALSE);
        float f;
        TBranch *branch2 = tree->Branch("myFloat", &f, 32000, 1);
        branch2->SetAutoDelete(kFALSE);
        ss.b = 2;
        ss.c = 3;
        for (int ev = 0; ev < 10; ev++) {
          ss.a = ev+1;
          f = ss.a+1;
          tree->Fill();
        }
        hfile = tree->GetCurrentFile();
        hfile->Write();
        tree->Print();
    } else {
        fprintf(stderr, "Unknown command: %s.\n", argv[1]);
        return 1;
    }
    hfile->Close();

    return 0;
}
