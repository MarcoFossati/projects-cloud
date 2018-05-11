// PASTA_1.1.cpp : Defines the entry point for the console application.
//
//###################################################################################
//
//	Program:		Panel-Method Aerothermodynamic Simulation and Thermal Analysis (B. Parsonage, M. Kelly, E. Kerr - 2018)
//	Version:		1.2 (Inclues function modules)
//	Input:			Input_PASTA.txt
//	Output:			<Main Directory>/Output Files/<date+time><STLname>.txt
//					<Main Directory>/Output Files/<date+time><STLname>.vtk
//	Notes:
//	Changelog:		B. Parsonage (2/1/2018) - Prepared v1.0 for upload
//					B. Parsonage (9/1/2018) - Converted aero/atmos modules into functions
//											- Added supersonic module
//					B. Parsonage (13/1/2018)- Converted geometry/function modules into functions
//
//	Code Structure:		INITIALISATION							- Defines necessary header files and variables
//						INPUT MODULE							- Reads Input_PASTA.txt
//						GEOMETRY MODULE							- Reads the binary STL file
//						ATMOSPHERE MODULE						- Computes atmospheric constants
//						SUBSONIC/SUPERSONIC/HYPERSONIC MODULE	- Evaluates Aerodynamic/Aerotherodynamic coefficients and distribution data
//						OUTPUT MODULE							- Writes a .txt file of scalar data, and a .vtk file of distribution data
//
//
//
//###################################################################################



//###################################################################################
//	INITIALISATION
//###################################################################################

//libraries
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

#include <cassert>


//Global variables
const double long PI = 3.141592653589793238462643383279502884;

const double SN = 1.0;							//Normal Momentum Accommodation coefficient
const double ST = 1.0;							//Tangential Momentum Accommodation Coefficient
const double AT = ST * (2 - ST);				//Tangental Energy Accomodation Coefficient
const double AN = 1.0 - pow((1.0 - SN), 2);		//Normal Energy Accommodation Coefficient
const double AC = 0.5 * (AN + AT);				//Overall Energy Accommodation Coefficient

const int hmin = 0;		//km					//minimum altitude present in the database : 0km
const int hmax = 999;	//km					//maximum altitude present in the database : 999km
const int Hcont = 40;							//Fixed reference continuum altitude
const int Hfmf = 220;							//Fixed reference Free molecular flow altitude
const double limKn_inf = 1E-4;					//fixed reference Kn continuum limit
const int limKn_sup = 10;						//fixed reference Kn FMF limit

std::string mainDir, gDir, STLname;
double altitude, Vinf, alpha, Beta, lref, Sref, Twi, rN;
std::vector<double> Vinfi, Vinfni, r;
int atmosflag1, atmosflag2;

std::vector<std::vector<double>> normals;		//create array of normal vectors
std::vector<std::vector<double>> vertices;		//create array of vertex vectors
std::vector<double> areas;						//create array of triangle areas
std::vector<std::vector<double>> COG;			//create array of COG coordinates 
std::vector<double> CG;
std::vector<std::vector<double>> incentres;		//create array of incentres

std::vector<double> Cc(3), Cfm(3), Mc(3), Mfm(3);
std::vector<double> Cfm_hfm(3), Mfm_hfm(3);

std::vector<std::vector<double>> B2WA(3);		//Body to Wind rotation matrix

unsigned int num_triangles;
const int num_columns = 3;

std::time_t start, mainstart;
double now, seconds;

//##############################################################################
//MODULES
//##############################################################################

#include "functions.h"
#include "geometry.h"
#include "atmosphere.h"
#include "hypersonic.h"
#include "supersonic.h"


//##############################################################################
//MAIN CODE
//##############################################################################

int main()
{
	mainstart = std::clock();
	//##################################################################################################
	//	INPUT MODULE - Reads input file
	//##################################################################################################

	//needed for reading input file
	std::ifstream input;
	std::string Strng1, Strng2, Strng3, Strng4, line;
	char chr1;

	input.open("Input_PASTA.txt");
	for (int lineno = 1; getline(input, line); lineno++) {
		if (lineno == 3) {
			getline(input, mainDir);
		}
		else if (lineno == 5) {
			getline(input, gDir);
		}
		else if (lineno == 9) {
			input >> Strng1 >> chr1 >> STLname;
		}
		else if (lineno == 16) {
			input >> Strng1 >> Strng2 >> chr1 >> altitude;
		}
		else if (lineno == 17) {
			input >> Strng1 >> Strng2 >> chr1 >> Vinf;
		}
		else if (lineno == 18) {
			input >> Strng1 >> Strng2 >> Strng3 >> Strng4 >> chr1 >> alpha;
		}
		else if (lineno == 19) {
			input >> Strng1 >> Strng2 >> Strng3 >> Strng4 >> chr1 >> Beta;
		}
		else if (lineno == 20) {
			input >> Strng1 >> Strng2 >> Strng3 >> Strng4 >> chr1 >> Twi;
		}
		else if (lineno == 27) {
			input >> Strng1 >> Strng2 >> Strng3 >> chr1 >> lref;
		}
		else if (lineno == 28) {
			input >> Strng1 >> Strng2 >> Strng3 >> Strng4 >> chr1 >> Sref;
		}
		else if (lineno == 29) {
			input >> Strng1 >> Strng2 >> Strng3 >> chr1 >> rN;
		}
		else if (lineno == 36) {
			input >> Strng1 >> Strng2 >> chr1 >> atmosflag1;
		}
		else if (lineno == 37) {
			input >> Strng1 >> Strng2 >> Strng3 >> Strng4 >> chr1 >> atmosflag2;
		}
	}
	input.close();

	//Error catching
	//<goes here>



	//##################################################################################################
	//	GEOMETRY MODULE - Reads any binary STL file
	//##################################################################################################

	std::vector<std::vector<std::vector<double>>> geometry = STLread3D(gDir, STLname);
	vertices = geometry[0], normals = geometry[1], COG = geometry[2], incentres = geometry[3];

	for (int t = 0; t < num_triangles; t++) {
		areas[t] = area(vertices[t * 3], vertices[t * 3 + 1], vertices[t * 3 + 2]);
	}

	CG = centreofgravity(incentres);

	//Find minimum X value
	double minX = 0;
	for (int t = 0; t<num_triangles;t++)
		if (COG[t][0] < minX) {
			minX = COG[t][0];
		}

	//###############################################################################################
	//	Free stream coordinate transformations
	//###############################################################################################	

	Vinfi = { Vinf * cosd(alpha) * cosd(Beta), -Vinf * sind(Beta), Vinf * sind(alpha) * cosd(Beta) }; //Free Stream Velocity Vector
	Vinfni = vecbyscal(Vinfi, (1 / Vinf)); //	Vinfi/Vinf	###FIX THIS LATER###

	//Body to Wind rotation matrix
	for (int i = 0; i < 3; ++i)
	{
		B2WA[i].resize(num_columns);
	}
	B2WA[0] = { cosd(alpha)*cosd(Beta), -sind(Beta), sind(alpha)*cosd(Beta) };
	B2WA[1] = { cosd(alpha)*sind(Beta), cosd(Beta), sind(alpha)*sind(Beta) };
	B2WA[2] = { -sind(alpha), 0, cosd(alpha) };


	//###############################################################################################
	//	BACK-FACE CULLING - Remove faces not seen by the flow
	//###############################################################################################

	start = std::clock();
	//BACKFACE CULLING
	//Really basic culling, removes face if x component of normal is negative (i.e. face is facing away from flow)
	std::vector<int> BFC(num_triangles);
	int num_triangles_culled = 0;
	double test;
	for (int t = 0;t < num_triangles;t++) {
		test = dot(normals[t], Vinfni) / (norm(normals[t])*norm(Vinfni)); //calculate cos(angle between normal and free stream velocity vector)
		if (test >= 0.0) {
			BFC[t] = 0;
		}
		else {
			BFC[t] = 1;
		}
		if (BFC[t] == 1) {
			num_triangles_culled = num_triangles_culled + 1;
		}
	}
	
	std::cout << "Geometry Initialisation... ";
	
	//HIDDEN SURFACE REMOVAL - Ray tracing
	start = std::clock();

	//centrepoint of each triangle not removed by BFC used to deterine starting point of each ray. Direction vector = Free Stream Velocity vector 
	std::vector<std::vector<double>> P_ray(num_triangles);
	for (int t = 0; t < num_triangles; t++) {
		if (BFC[t] == 1) {
			P_ray[t].resize(3);
			double td = (COG[t][0] - minX) / Vinfni[0];
			P_ray[t] = vec_elem_math(COG[t], vecbyscal(Vinfni, td), 2);
		}
	}

	//Calculate d for each triangle (remaining after BFC) (ax + by + cz = d <- equation of a plane)
	//d = normal * vertex (any vertex: A, B or C)
	std::vector<double> d(num_triangles);
	for (int t = 0; t < num_triangles; t++) {
		if (BFC[t] == 1) {
			d[t] = dot(normals[t], vertices[t * 3]);
		}
	}

	//Calculate intersection of each ray with each triangle plane (THIS COULD BE WAY MORE EFFICIENT)
	//		t = (d - dot(normal,P))/dot(normal, rayvector)		P = starting point of ray
	//		Rt = P + t*rayvector		Rt = coordinates of intersection
	std::vector<std::vector<double>> Rt(num_triangles);
	for (int t = 0; t < num_triangles; t++) {
		if (BFC[t] == 1) {
			Rt[t].resize(3);
		}
	}

	std::vector<int> HSR(num_triangles);
	int num_triangles_culled_HSR = 0, num_intersects, closest;
	for (int ray = num_triangles - 1; ray != -1; ray--) {
		num_intersects = 0;
		std::vector<int> intersects(100); //assuming there won't be more than 100 intersects
		if (BFC[ray] == 1) {
			for (int t = 0; t < num_triangles; t++) {
				if (BFC[t] == 1) {
					//if (HSR[t] == 0) {
					double td = (d[t] - dot(normals[t], P_ray[ray])) / dot(normals[t], Vinfni);
					Rt[t] = vec_elem_math(P_ray[ray], vecbyscal(Vinfni, td), 1);

					if (dot(normals[t], Vinfni) != 0) { //check if face is parallel to FSV (parallel if dot(normal,Vinfni) = 0)
						//Does intersection lie within the triangle?
						std::vector<double> A = vertices[t * 3];
						std::vector<double> B = vertices[t * 3 + 1];
						std::vector<double> C = vertices[t * 3 + 2];
						double test1 = dot(cross(vec_elem_math(B, A, 2), vec_elem_math(Rt[t], A, 2)), normals[t]);
						double test2 = dot(cross(vec_elem_math(C, B, 2), vec_elem_math(Rt[t], B, 2)), normals[t]);
						double test3 = dot(cross(vec_elem_math(A, C, 2), vec_elem_math(Rt[t], C, 2)), normals[t]);
						if (test1 >= 0) { //writing out the long way to try and make code run faster
							if (test2 >= 0) {
								if (test3 >= 0) {
									intersects[num_intersects] = t; //saves the NUMBER of the triangle that is intersected
									num_intersects = num_intersects + 1;
								}
							}
						}
					}
					else {
						intersects[num_intersects] = t;
						num_intersects = num_intersects + 1;
					}
					//}
				}
			}

			//find which of the intersects, if multiple, is closest to origin of the ray
			if (num_intersects != 0) {
				std::vector<int> test(num_intersects);
				for (int i = 0; i < num_intersects; i++) {
					test[i] = intersects[i];
				}

				//Test each inersection
				closest = test[0]; //starting assumption
				if (num_intersects == 1) {
					closest = test[0];
				}
				else {
					for (int i = 0; i < num_intersects - 1; i++) {
						if (Rt[test[i + 1]] < Rt[closest]) {
							closest = test[i + 1];
						}
						else {
							HSR[test[i + 1]] = 2;
						}
					}
				}
				if (HSR[closest] != 1) {
					HSR[closest] = 1;
					num_triangles_culled_HSR = num_triangles_culled_HSR + 1;
				}
			}
		}
	}

	seconds = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << seconds << " seconds\n";
	std::cout << "\n";
	std::cout << "Total number of faces: " << num_triangles << "\n";
	std::cout << "Number of rays: " << num_triangles_culled << "\n";
	std::cout << "Number of faces after HSR: " << num_triangles_culled_HSR << "\n" << "\n";

	//###############################################################################################
	//	ATMOSPHERE MODULE
	//###############################################################################################
	std::vector<double> ATMOS;
	double P, T, rho, Minf, R, cp, gamma, Re0, Kn, T0, P01, s, Cpmax;
	if (atmosflag1 == 1) {
		ATMOS = NRLMSISE00(altitude);
		P = ATMOS[0], T = ATMOS[1], rho = ATMOS[2], Minf = ATMOS[3], R = ATMOS[4], cp = ATMOS[5], gamma = ATMOS[6], Re0 = ATMOS[7], Kn = ATMOS[8], T0 = ATMOS[9], P01 = ATMOS[10], s = ATMOS[11], Cpmax = ATMOS[12];
	}
	if (atmosflag2 == 1) {
		ATMOS = USSA76(altitude);
		P = ATMOS[0], T = ATMOS[1], rho = ATMOS[2], Minf = ATMOS[3], R = ATMOS[4], cp = ATMOS[5], gamma = ATMOS[6], Re0 = ATMOS[7], Kn = ATMOS[8], T0 = ATMOS[9], P01 = ATMOS[10], s = ATMOS[11], Cpmax = ATMOS[12];
	}

	//###############################################################################################
	//	SIMULATION INITIALISATION 
	//###############################################################################################

	std::cout << "Simulation Started\n";

	//Initialise variables that are needed outside the triangles 'for' loop
	double Qav_c, Qav_fm, Ch_c_av, Ch_fm_av;
	double CDc, CSc, CLc, Mlc, Mmc, Mnc;
	double CDfm, CSfm, CLfm, Mlfm, Mmfm, Mnfm;
	double Mltrans, Mmtrans, Mntrans, CDtrans, CStrans, CLtrans, test1;
	double CDss, CLss, CSss;
	std::vector<double> Q_c(num_triangles), Total_heat_c(num_triangles), Stcfr_FOSTRAD20(num_triangles), Chc(num_triangles), Cpc(num_triangles);
	std::vector<double> Q_fm(num_triangles), Total_heat_fm(num_triangles), Stfm1(num_triangles), Chfm(num_triangles), Cpfm(num_triangles);


	//###############################################################################################
	//	AERODYNAMICS MODULE
	//###############################################################################################

	if (Minf >= 5) {
		std::vector<std::vector<double>> Aero = Aero_hyp(ATMOS, HSR);
		CDc = Aero[2][0], CSc = Aero[2][1], CLc = Aero[2][2], Mlc = Aero[2][6], Mmc = Aero[2][7], Mnc = Aero[2][8];
		CDfm = Aero[2][3], CSfm = Aero[2][4], CLfm = Aero[2][5], Mlfm = Aero[2][9], Mmfm = Aero[2][10], Mnfm = Aero[2][11];
		Cpc = Aero[0], Cpfm = Aero[1];
	}
	else if ((Minf > 1) && (Minf < 5)) {
		std::vector<std::vector<double>> Aero = Aero_ss(ATMOS, HSR);
		CLss = Aero[1][0], CDss = Aero[1][1], CSss = Aero[1][2];
		Cpc = Aero[0];
	}

	//###############################################################################################
	//	AEROTHERMODYNAMICS MODULE
	//###############################################################################################

	if (Minf >= 5) {
		if (Kn < limKn_inf) {
			std::vector<std::vector<double>> Aerothermo = Aerothermo_hyp(ATMOS, HSR);
			Total_heat_c = Aerothermo[0], Q_c = Aerothermo[1], Chc = Aerothermo[2];

			double sumQ_c = 0;
			double sumCh_c = 0;
			double Atot = 0;
			for (int i = 0; i < num_triangles;i++) {
				sumQ_c = sumQ_c + Total_heat_c[i];
				sumCh_c = sumCh_c + Chc[i];
				Atot = Atot + areas[i];
				Qav_c = sumQ_c / Atot;							//average heat transfer [W/m^2]
			}
			Ch_c_av = sumCh_c / num_triangles;
		}
		else if (Kn >= limKn_sup) {
			std::vector<std::vector<double>> Aerothermo = Aerothermo_hyp(ATMOS, HSR);
			Total_heat_fm = Aerothermo[0], Q_fm = Aerothermo[1], Chfm = Aerothermo[2];

			double sumQ_fm = 0;
			double sumCh_fm = 0;
			double Atot = 0;
			for (int i = 0; i < num_triangles;i++) {
				sumQ_fm = sumQ_fm + Total_heat_fm[i];
				sumCh_fm = sumCh_fm + Chfm[i];
				Atot = Atot + areas[i];
				Qav_fm = sumQ_fm / Atot;						//average heat transfer [W/m^2]
			}
			Ch_fm_av = sumCh_fm / num_triangles;
		}

	}

	//###############################################################################################
	//	TRANSITION BRIDGING MODULE - Evaluated using values for Continuum and FM flow
	//###############################################################################################

	//Find continuum and FM limits (highest/lowest altitude where flow is continuum/FM)
	double guess = Hcont;
	std::vector<double> ATMOS_guess = NRLMSISE00(guess);
	double Kn_guess = ATMOS_guess[8];
	double step = 10;
	int direction = 1; //1 = up, -1 = down;
	int prev_direction = 1;
	while (Kn_guess != limKn_inf) {
		if (Kn_guess < limKn_inf) {
			prev_direction = direction;
			direction = 1;
		}
		else {
			prev_direction = direction;
			direction = -1;
		}
		if (prev_direction != direction) {
			step = step / 2;
		}
		guess = guess + (direction*step);
		ATMOS_guess = NRLMSISE00(guess);
		Kn_guess = ATMOS_guess[8];
		if ((Kn_guess >= 0.95*limKn_inf) && (Kn_guess < limKn_inf)) {
			break;
		}
	}
	double limit_c = guess;

	double limit_fm;
	if (altitude > Hfmf) {
		limit_fm = altitude;
	}
	else {
		limit_fm = Hfmf;
	}



	//Continuum limit atmosphere calculations
	std::vector<double> ATMOS_c = NRLMSISE00(limit_c);
	double Re0_hc = ATMOS_c[7], Cpmax_hc = ATMOS_c[12];
	double CDc_hc, CSc_hc, CLc_hc, Mlc_hc, Mmc_hc, Mnc_hc;
	std::vector<double> Cpc_hc(num_triangles);

	//Free Molecular limit atmosphere calculations
	std::vector<double> ATMOS_fm = NRLMSISE00(limit_fm);
	double T_hfm = ATMOS_fm[1], rho_hfm = ATMOS_fm[2], gamma_hfm = ATMOS_fm[6], T0_hfm = ATMOS_fm[9], s_hfm = ATMOS_fm[11];
	double CDfm_hfm, CSfm_hfm, CLfm_hfm, Mlfm_hfm, Mmfm_hfm, Mnfm_hfm;
	std::vector<double> Cpfm_hfm(num_triangles);

	//Transitional Aerodynamics 
	if ((Kn >= limKn_inf) && (Kn <= limKn_sup)) {

		std::vector<std::vector<double>> Aero_fm, Aero_c;

		//Continuum
		Aero_c = Aero_hyp(ATMOS_c, HSR);
		CDc_hc = Aero_c[2][0], CSc_hc = Aero_c[2][1], CLc_hc = Aero_c[2][2], Mlc_hc = Aero_c[2][6], Mmc_hc = Aero_c[2][7], Mnc_hc = Aero_c[2][8];
		Cpc_hc = Aero_c[0];

		//Free Molecular
		Aero_fm = Aero_hyp(ATMOS_fm, HSR);
		CDfm_hfm = Aero_fm[2][3], CSfm_hfm = Aero_fm[2][4], CLfm_hfm = Aero_fm[2][5], Mlfm_hfm = Aero_fm[2][9], Mmfm_hfm = Aero_fm[2][10], Mnfm_hfm = Aero_fm[2][11];
		Cpfm_hfm = Aero_fm[1];

		/*
		//Transitional (LEGGE FORMUALA)
		Mltrans = Mlc_hc + (Mlfm_hfm - Mlc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		Mmtrans = Mmc_hc + (Mmfm_hfm - Mmc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		Mntrans = Mnc_hc + (Mnfm_hfm - Mnc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		CDtrans = CDc_hc + (CDfm_hfm - CDc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		CStrans = CSc_hc + (CSfm_hfm - CSc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		CLtrans = CLc_hc + (CLfm_hfm - CLc_hc) * pow((sin(PI*(3.5 / 8 + 0.125*log10(Kn)))), 2);
		*/

		//Transitional (5PL weighted least squares regression)
		//double a = 0, b = 0.876305393, c = 0.054034346, d = 1, e = 0.568196706; //coefficients calculated using 5PL for Orion geometry - Don't know if valid for otehr geometry
		double a = 0, b = 0.7, c = 0.06, d = 1, e = 0.55;
		Mltrans = Mlc_hc + (Mlfm_hfm - Mlc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		Mmtrans = Mmc_hc + (Mmfm_hfm - Mmc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		Mntrans = Mnc_hc + (Mnfm_hfm - Mnc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CDtrans = CDc_hc + (CDfm_hfm - CDc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CStrans = CSc_hc + (CSfm_hfm - CSc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CLtrans = CLc_hc + (CLfm_hfm - CLc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));

	}

	//Define variables used for Transitional Aerothermodynamic calculations
	std::vector<double> Qtrans(num_triangles), Total_heat_trans(num_triangles), Stctrans(num_triangles), Stctrans1(num_triangles), Chtrans(num_triangles);
	double Qav_t, Total_heat_t_av, Ch_t_av;

	//Transitional Aero-Thermodynamics 
	if ((Kn >= limKn_inf) && (Kn <= limKn_sup)) {

		double a = 0, b = 0.86997932, c = 0.059205299, d = 1, e = 0.619808542; //coefficients calculated using 5PL for Orion geometry - Don't know if valid for otehr geometry

		//Continuum
		std::vector<std::vector<double>> Aerothermo_c = Aerothermo_hyp(ATMOS_c, HSR);
		Stcfr_FOSTRAD20 = Aerothermo_c[2];

		//Free-Molecular
		std::vector<std::vector<double>> Aerothermo_fm = Aerothermo_hyp(ATMOS_fm, HSR);
		Stfm1 = Aerothermo_fm[2];

		//Transition (Using Continuum and Free Molecular values)
		for (int t = 0; t < num_triangles; t++) {
			if (HSR[t] == 1) {
				Stctrans[t] = (Stcfr_FOSTRAD20[t] + Kn * Stfm1[t]) / (1 + Kn);		//need to have these evaluated
				//Stctrans1[t] = Stcfr_FOSTRAD20[t] / (sqrt(1 + pow(Stcfr_FOSTRAD20[t] / Stfm1[t], 2)));
				Stctrans1[t] = Stcfr_FOSTRAD20[t] + (Stfm1[t] - Stcfr_FOSTRAD20[t]) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
				Qtrans[t] = Stctrans1[t] * rho * Vinf * cp * (T0 - Twi);
				Total_heat_trans[t] = Qtrans[t] * areas[t];
				Chtrans[t] = 2 * Qtrans[t] / (rho*pow(Vinf, 3));					//Heat Transfer Coefficient
			}
		}
		double sumQ_t = 0;
		double sumCh_t = 0;
		double Atot = 0;
		for (int i = 0; i < num_triangles;i++) {
			sumQ_t = sumQ_t + Total_heat_trans[i];
			sumCh_t = sumCh_t + Chtrans[i];
			Atot = Atot + areas[i];
			Qav_t = sumQ_t / Atot;							//average heat transfer [W/m^2]
		}
		Total_heat_t_av = sumQ_t / num_triangles;
		Ch_t_av = sumCh_t / num_triangles;
	}

	//##################################################################################
	//Compile final values
	//##################################################################################
	std::vector<double> Q(num_triangles);				//W/m2
	std::vector<double> Total_heat(num_triangles);		//W
	std::vector<double> Ch(num_triangles);
	std::vector<double> Cp(num_triangles);
	double CL, CD, CS, Qav = 0, Ml, Mm, Mn, Ch_av = 0;
	

	std::vector<double> ATMOS_c_global = NRLMSISE00(Hcont);
	std::vector<std::vector<double>> Aero_c_global = Aero_hyp(ATMOS_c_global, HSR);
	CDc_hc = Aero_c_global[2][0], CSc_hc = Aero_c_global[2][1], CLc_hc = Aero_c_global[2][2], Mlc_hc = Aero_c_global[2][6], Mmc_hc = Aero_c_global[2][7], Mnc_hc = Aero_c_global[2][8];
	Cpc_hc = Aero_c_global[0];

	std::vector<double> ATMOS_fm_global = NRLMSISE00(Hfmf);
	std::vector<std::vector<double>> Aero_fm_global = Aero_hyp(ATMOS_fm_global, HSR);
	CDfm_hfm = Aero_fm_global[2][3], CSfm_hfm = Aero_fm_global[2][4], CLfm_hfm = Aero_fm_global[2][5], Mlfm_hfm = Aero_fm_global[2][9], Mmfm_hfm = Aero_fm_global[2][10], Mnfm_hfm = Aero_fm_global[2][11];
	Cpfm_hfm = Aero_fm_global[1];
		

	if (Minf >= 5) {

		//Global smoothing function
		double a = 0.0, b = 0.5658, c = 0.35, d = 1.017, e = 1.2; //coefficients calculated using 5PL for Orion geometry - Don't know if valid for other geometry

		Ml = Mlc_hc + (Mlfm_hfm - Mlc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		Mm = Mmc_hc + (Mmfm_hfm - Mmc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		Mn = Mnc_hc + (Mnfm_hfm - Mnc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CD = CDc_hc + (CDfm_hfm - CDc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CS = CSc_hc + (CSfm_hfm - CSc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		CL = CLc_hc + (CLfm_hfm - CLc_hc) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));

		for (int t = 0; t < num_triangles; t++) {
		Cp[t] = Cpc_hc[t] + (Cpfm_hfm[t] - Cpc_hc[t]) * (d + (a - d) / (pow(1 + pow(Kn / c, b), e)));
		}


		if (Kn < limKn_inf) {
			Q = Q_c; Total_heat = Total_heat_c; Ch = Chc;
			//CL = CLc; CD = CDc; CS = CSc;
			//Ml = Mlc; Mm = Mmc; Mn = Mnc;
			Qav = Qav_c; Ch_av = Ch_c_av;
		}
		else if ((limKn_inf <= Kn) && (Kn <= limKn_sup)) {
			Q = Qtrans; Total_heat = Total_heat_trans; Ch = Chtrans;
			//CL = CLtrans; CD = CDtrans; CS = CStrans;
			//Ml = Mltrans; Mm = Mmtrans; Mn = Mntrans;
			Qav = Qav_t; Ch_av = Ch_t_av;
		}
		else if (Kn > limKn_sup) {
			Q = Q_fm; Total_heat = Total_heat_fm; Ch = Chfm;
			//CL = CLfm; CD = CDfm; CS = CSfm;
			//Ml = Mlfm; Mm = Mmfm; Mn = Mnfm;
			Qav = Qav_fm; Ch_av = Ch_fm_av;
		}
	}
	else if ((Minf > 1) && (Minf < 5)) {
		CL = CLss;
		CD = CDss;
		CS = CSss;
	}

	//###############################################################################################
	//	OUTPUT MODULE
	//###############################################################################################

	//Get date and time
	std::time_t now = time(0);
	struct tm* dt = localtime(&now);
	char buffer[160];
	std::strftime(buffer, 160, "%d-%m-%Y %H.%M", dt);

	//Write to console
	std::cout << "\n";
	std::cout << "~~~~~~~~~~~~~~~~~~~~ P.A.S.T.A v1.2 Output File - " << buffer << " ~~~~~~~~~~~~~~~~~~~~\n";
	std::cout << "\n";
	std::cout << "STL File:			" << STLname << "\n";
	std::cout << "\n";
	std::cout << "INPUTS:\n";
	std::cout << "Altitude (km):			" << altitude << "\n";
	std::cout << "Velocity (m/s):			" << Vinf << "\n";
	std::cout << "Angle of Attack (deg):		" << alpha << "\n";
	std::cout << "Angle of Sideslip (deg):	" << Beta << "\n";
	std::cout << "Fixed Wall Temp (K):		" << Twi << "\n";
	std::cout << "Reference Cross Section (m2):	" << Sref << "\n";
	std::cout << "Reference Length (m):		" << lref << "\n";
	std::cout << "Nose Radius (m):		" << rN << "\n";
	std::cout << "\n";
	std::cout << "OUTPUTS:\n";
	std::cout << "Mach Number:			" << Minf << "\n";
	std::cout << "Knudsen Number:			" << Kn << "\n";
	std::cout << "Reynolds Number:		" << Re0 << "\n";
	std::cout << "Coefficient of Drag:		" << CD << "\n";
	std::cout << "Coefficient of Lift:		" << CL << "\n";
	std::cout << "Coefficient of Sideslip:	" << CS << "\n";
	//std::cout << "Rolling Moment Coefficient:	" << Ml << "\n";
	//std::cout << "Pitching Moment Coefficient:	" << Mm << "\n";
	//std::cout << "Yaw Moment Coefficient:		" << Mn << "\n";
	std::cout << "Av. Heat Transfer Flux (W/m2):	" << Qav << "\n";
	std::cout << "Av. Heat Transfer Coefficient:	" << Ch_av << "\n";
	std::cout << "\n";
	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
	std::cout << "\n";

	//Create and write output file
	std::ofstream output;

	output.open(mainDir + "Output Files/" + std::string(buffer) + " - " + STLname + ".txt");
	output << "~~~~~~~~~~~~~~~~~~~~ P.A.S.T.A v1.2 Output File - " << buffer << " ~~~~~~~~~~~~~~~~~~~~\n";
	output << "\n";
	output << "STL File:			" << STLname << "\n";
	output << "\n";
	output << "INPUTS:\n";
	output << "Altitude (km):			" << altitude << "\n";
	output << "Velocity (m/s):			" << Vinf << "\n";
	output << "Angle of Attack (deg):		" << alpha << "\n";
	output << "Angle of Sideslip (deg):	" << Beta << "\n";
	output << "Fixed Wall Temp (K):		" << Twi << "\n";
	output << "Reference Cross Section (m2):	" << Sref << "\n";
	output << "Reference Length (m):		" << lref << "\n";
	output << "Nose Radius (m):		" << rN << "\n";
	output << "\n";
	output << "OUTPUTS:\n";
	output << "Mach Number:			" << Minf << "\n";
	output << "Knudsen Number:			" << Kn << "\n";
	output << "Reynolds Number:		" << Re0 << "\n";
	output << "Coefficient of Drag:		" << CD << "\n";
	output << "Coefficient of Lift:		" << CL << "\n";
	output << "Coefficient of Sideslip:	" << CS << "\n";
	//output << "Rolling Moment Coefficient:	" << Ml << "\n";
	//output << "Pitching Moment Coefficient:	" << Mm << "\n";
	//output << "Yaw Moment Coefficient:		" << Mn << "\n";
	output << "Av. Heat Transfer Flux (W/m2):	" << Qav << "\n";
	output << "Av. Heat Transfer Coefficient:	" << Ch_av << "\n";
	output << "\n";
	output << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
	output << "\n";

	output.close();

	//Create VTK file
	std::ofstream vtkfile;
	vtkfile.open(mainDir + "Output Files/" + std::string(buffer) + " - " + STLname + ".vtk");
	vtkfile << "# vtk DataFile Version 3.0\n";
	vtkfile << "PASTA 1.1 VTK file - " << buffer << "\n";
	vtkfile << "ASCII\n";
	vtkfile << "DATASET POLYDATA\n";
	vtkfile << "POINTS " << num_triangles * 3 << " " << "float" << "\n";
	for (int x = 0; x < num_triangles * 3; x++)
	{
		for (int y = 0; y < num_columns; y++)
		{
			vtkfile << vertices[x][y] << " ";
		}
		vtkfile << "\n";
	}
	vtkfile << "\n";

	vtkfile << "POLYGONS " << num_triangles << " " << num_triangles * 4 << "\n";
	for (int x = 0; x < num_triangles; x++)
	{
		vtkfile << "3 ";
		vtkfile << (3 * x) << " ";
		vtkfile << (3 * x + 1) << " ";
		vtkfile << (3 * x + 2) << " ";
		vtkfile << "\n";
	}
	vtkfile << "\n";


	vtkfile << "CELL_DATA " << num_triangles << "\n";
	vtkfile << "SCALARS Q[W/m2] double 1\n";
	vtkfile << "LOOKUP_TABLE default\n";
	for (int x = 0; x < num_triangles; x++)
	{
		vtkfile << Q[x] << "\n";
	}
	vtkfile << "\n";

	vtkfile << "SCALARS Ch double 1\n";
	vtkfile << "LOOKUP_TABLE default\n";
	for (int x = 0; x < num_triangles; x++)
	{
		vtkfile << Ch[x] << "\n";
	}
	vtkfile << "\n";

	vtkfile << "SCALARS Heat_Transfer[W] double 1\n";
	vtkfile << "LOOKUP_TABLE default\n";
	for (int x = 0; x < num_triangles; x++)
	{
		vtkfile << Total_heat[x] << "\n";
	}
	vtkfile << "\n";

	vtkfile << "SCALARS Cp double 1\n";
	vtkfile << "LOOKUP_TABLE default\n";
	for (int x = 0; x < num_triangles; x++)
	{
		vtkfile << Cp[x] << "\n";
	}
	vtkfile << "\n";

	vtkfile.close();

	seconds = (std::clock() - mainstart) / (double)CLOCKS_PER_SEC;

	std::cout << "Simulation Complete\n";
	std::cout << "\n";
	std::cout << "Total run-time = " << seconds << " seconds\n";
	std::cout << "\n";
	std::cout << "Press ENTER to continue...";
	getchar();  // Used to keep command window open afetr running .exe file


	return 0;
} //END OF MAIN


