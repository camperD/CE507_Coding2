//   Created by Damyn Chipman on 10/31/18
//   github: @camperD
//   Copyright © 2018 Damyn Chipman. All rights reserved.
//      FILE:   FE1D.h
//   PROJECT:   CE507_Coding2

#ifndef FE1D_h
#define FE1D_h

// ----- Necessary Files -----
#include <vector>
#include <Eigen/Sparse>
#include <Eigen/Dense>

// ----- Project Files -----
#include "BSpline.h"
#include "DBSpline.h"

using namespace Eigen;
using namespace std;

typedef Eigen::SparseMatrix<float> SpMat;
typedef Eigen::Triplet<float> triplet;

Eigen::VectorXf FE1D(int** LM, int p, int NE, int NINT, float (*f)(float), Eigen::VectorXf knotVector, int NKNOTS, Eigen::VectorXf w, Eigen::VectorXf intPoints, float del_e) {

    // Initialize matrices
    std::vector<triplet> coefs;
    Eigen::VectorXf F(NE);
    for (int i = 0; i < NE; i++) {
        F(i) = 0;
    }
    
    // Necessary Variables
    Eigen::MatrixXf k_ab(p+1,p+1);
    Eigen::VectorXf f_a(p+1);
    
    // ----- Begin Assembly Algorithm -----
    if (VERBOSE) { cout << "---------- BEGINNING ASSEMBLY ALGORITHM ----------" << endl; }
    for (int e = 0; e < NE; e++) {
        
        // Initialize element matrix and vector
        k_ab = Eigen::MatrixXf::Zero(p+1,p+1);
        f_a = Eigen::VectorXf::Zero(p+1);
        
        if (VERBOSE) { cout << "  Element: " << e << endl << endl; }
        
        for (int i = 0; i < NINT; i++) {
            if (VERBOSE) { cout << "    Integration point index: " << i << endl << endl; }
            // Compute Bpi
            BSpline Bpi(p, intPoints(i));
            Eigen::VectorXf Bpi_vec = Bpi.getVector();
            
            // Compute Np
            Eigen::VectorXf Np_vec = C_P(p, e)*Bpi_vec;
            
            // Compute DBpi
            DBSpline DBpi(p, intPoints(i));
            Eigen::VectorXf DBpi_vec = DBpi.getVector();
            
            // Compute DNp
            Eigen::VectorXf DNp_vec = C_P(p, e)*DBpi_vec;
            
            // Compute x(xi_i)
            float x_xi = 0.0;
            for (int A = 0; A < p+1; A++) {
                
                // Calc x_A
                float x_A = 0.0;
                for (int j = A+1; j < A + p + 1; j++) {
                    x_A = x_A + knotVector(j+e);
                }
                
                // Calc x
                if (p != 1) {
                    x_xi = x_xi + (1.0/p)*x_A*Np_vec(A);
                }
                else {
                    x_xi = x_xi + (1.0/p)*x_A*Bpi_vec(A);
                }
            }
            
            
            // Compute f_i = f(x(xi_i))
            float f_i = f(x_xi);
            
            // Assemble
            for (int a = 0; a < p+1; a++) {
                for (int b = 0; b < p+1; b++) {
                    // Compute and update k_e_ab
                    if (p != 1) {
                        k_ab(a,b) = k_ab(a,b) + DNp_vec(a)*DNp_vec(b)*(2.0/del_e)*w(i);
                    }
                    else { // Linear, no change in k_ab_e
                        if (a == b) {
                            k_ab(a,b) = 1/del_e;
                        }
                        else {
                            k_ab(a,b) = -1/del_e;
                        }
                    }
                }
                // Compute and update f_e_a
                f_a(a) = f_a(a) + Np_vec(a)*f_i*(del_e/2.0)*w(i);
            }
            
            if (VERBOSE) { // Print out
                cout << "      B_array:  [";
                for(int n = 0; n < p+1; n++) { cout << Bpi_vec(n) << " "; }
                cout << "]" << endl;
                cout << "      dB_array: [";
                for(int n = 0; n < p+1; n++) { cout << DBpi_vec(n) << " "; }
                cout << "]" << endl;
                cout << "      N_array:  [";
                for(int n = 0; n < p+1; n++) { cout << Np_vec(n) << " "; }
                cout << "]" << endl;
                cout << "      dN_array: [";
                for(int n = 0; n < p+1; n++) { cout << DNp_vec(n) << " "; }
                cout << "]" << endl;
                cout << "      x:         " << x_xi << endl;
                cout << "      f(x):      " << f_i << endl;
                cout << "      f_a: " << endl << f_a << endl << endl;
                cout << "      k_ab: " << endl << k_ab << endl << endl;
            }
        }
        for (int a = 0; a < p+1; a++) {
            int P = LM[a][e];
            for (int b = 0; b < p+1; b++) {
                int Q = LM[b][e];
                if (P != -1 && Q != -1) {
                    // Compute and update K_LM[a][e]_LM[a][e]
                    coefs.push_back(triplet(P,Q,k_ab(a,b)));
                }
            }
            if (P != -1) {
                // Compute and update F_LM[a][e]
                F(P) = F(P) + f_a(a);
            }
        }
    }
    // ----- End Assembly Algorithm -----
    if (VERBOSE) { cout << "---------- END OF ASSEMBLY ALGORITHM ----------" << endl; }
    
    SpMat K(NE,NE);
    K.setFromTriplets(coefs.begin(), coefs.end());
    Eigen::SimplicialCholesky<SpMat> chol(K);
    Eigen::VectorXf d = chol.solve(F);
    d(NE-1) = 0.0;
    
    if (VERBOSE) {
        cout << endl << "K: " << endl << K << endl;
        cout << endl << "F: " << endl << F << endl;
        cout << endl << "d: " << endl << d << endl;
    }
    
    return d;
}

#endif /* FE1D_h */
