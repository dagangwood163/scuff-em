/* Copyright (C) 2005-2011 M. T. Homer Reid
 *
 * This file is part of SCUFF-EM.
 *
 * SCUFF-EM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SCUFF-EM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <math.h>

#include <libhrutil.h>
#include <libMDInterp.h>
#include <libscuff.h>
#include <libscuffInternals.h>

#define II cdouble(0,1)

namespace scuff{

/***************************************************************/
/* entry point for GBarVD that has the proper prototype for    */
/* passage to the Interp2D() initialization routine.           */
/***************************************************************/
void GBarVDPhi2D(double x, double Rho, void *UserData, double *PhiVD)
{
  GBarData *GBD = (GBarData *)UserData;
 
  double R[3];
  R[0]=x;
  R[1]=Rho;
  R[2]=0.0;

  cdouble GBarVD[8];
  GBarVDEwald(R, GBD->k, GBD->kBloch, GBD->LBV, GBD->LDim,
              -1.0, GBD->ExcludeInnerCells, GBarVD);
 
  PhiVD[0] = real(GBarVD[0]); // real(G)
  PhiVD[1] = real(GBarVD[1]); // real(dGdX)
  PhiVD[2] = real(GBarVD[2]); // real(dGdY) = real(dGdRho)
  PhiVD[3] = real(GBarVD[4]); // real(dG2dXdY) = real(dG2dXdRho)

  PhiVD[4] = imag(GBarVD[0]); // imag(G)
  PhiVD[5] = imag(GBarVD[1]); // imag(dGdX)
  PhiVD[6] = imag(GBarVD[2]); // imag(dGdRho)
  PhiVD[7] = imag(GBarVD[4]); // imag(dG2dXdRho)

}

/***************************************************************/
/***************************************************************/
/***************************************************************/
GBarAccelerator *CreateGBarAccelerator(int LDim, double *LBV[2],
                                       double RhoMin, double RhoMax,
                                       cdouble k, double *kBloch)
{
  GBarAccelerator *GBA = (void *)mallocEC(sizeof(*GBA));

  GBA->k                 = k;
  GBA->kBloch            = kBloch;
  GBA->ExcludeInnerCells = ExcludeInnerCells;

  GBA->LDim = LDim;
  GBA->LBV1[0]=LBV[0][0];
  GBA->LBV1[1]=LBV[0][1];
  if (LDim==2)
   { GBA->LBV2[0]=LBV[1][0];
     GBA->LBV2[1]=LBV[1][1];
   };
  GBA->LBV[0] = GBA->LBV1;
  GBA->LBV[1] = GBA->LBV2;

  if (LDim==1)
   {
      int nx; 
      int nRho;

      GBA->I3D=0;
      GBA->I2D=new Interp2D(0.0, LBV[0][0], nx,
                            RhoMin, RhoMax, nRho,
                            2, GBarVDPhi2D, (void *)GBA);
   }
  else // LDim==2
   {
      GBA->I2D=0;
      GBA->I3D=new Interp3D(0.0, LBV[0][0], nx,
                            RhoMin, RhoMax, nRho,
                            2, GBarVDPhi2D, (void *)GBA);
   };

  return GBA;
   
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetGBarVD(double R[3], GBarAccelerator *GBA, cdouble GBarVD[4])
{
  int LDim               = GBA->LDim;
  double **LBV           = GBA->LBV;
  cdouble k              = GBA->k;
  double *kBloch         = GBA->kBloch;
  bool ExcludeInnerCells = GBA->ExcludeInnerCells;
 
  if (GBA->LDim==1)
   { 
     // get xbar = unit-cell representative of x coordinate
     double L0 = GBA->LBV[0][0];
     int m = (int)(floor( R[0] / L0 ));
     double xBar = R[0] - m*L0;
     double Rho = sqrt(R[1]*R[1] + R[2]*R[2]);

     double Phi[8];
     GBA->I2D->EvaluatePlus(xBar, Rho, Phi);
     cdouble G         = cdouble(Phi[0], Phi[4]);
     cdouble dGdx      = cdouble(Phi[1], Phi[5]);
     cdouble dGdRho    = cdouble(Phi[2], Phi[6]);

     // correct for the fact that we may have needed to 
     // translate the evaluation point into the unit cell
     if (m!=0)
      { 
        cdouble PhaseFactor = exp( II*m*kBloch[0]*L0 );

        if (ExcludeInnerCells)
         { 
           if (m==1) 
   
         };

        G*=PhaseFactor;
        dGdx*=PhaseFactor;
        dGdRho*=PhaseFactor;
        
      };

     GBarVD[0] = G;
     GBarVD[1] = dGdx;
     GBarVD[2] = (R[1]/Rho) * dGdRho;
     GBarVD[3] = (R[2]/Rho) * dGdRho;

   }
  else // (LDim==2)
   { 
   };
  
}

} // namespace scuff
