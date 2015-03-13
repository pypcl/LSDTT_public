

#ifndef LSDBasin_CPP
#define LSDBasin_CPP

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>
#include "TNT/tnt.h"
#include "LSDFlowInfo.hpp"
#include "LSDRaster.hpp"
#include "LSDIndexRaster.hpp"
#include "LSDIndexChannel.hpp"
#include "LSDJunctionNetwork.hpp"
#include "LSDStatsTools.hpp"
#include "LSDBasin.hpp"
#include "LSDParticle.hpp"
#include "LSDCRNParameters.hpp"
using namespace std;
using namespace TNT;

void LSDBasin::create()
{
  
  
  // now we set up empty variables to store properties of the basin
  // these are populated as they are required using the set methods in LSDBasin
    
  SlopeMean = NoDataValue;
  ElevationMean = NoDataValue;
  AspectMean = NoDataValue;
  ReliefMean = NoDataValue;
  PlanCurvMean = NoDataValue;
  ProfileCurvMean = NoDataValue;
  TotalCurvMean = NoDataValue;
  PlanCurvMax = NoDataValue;
  ProfileCurvMax = NoDataValue;
  TotalCurvMax = NoDataValue;
  HillslopeLength_HFR = NoDataValue;
  HillslopeLength_Binned = NoDataValue;
  HillslopeLength_Spline = NoDataValue;
  HillslopeLength_Density = NoDataValue;
  FlowLength = NoDataValue;
  DrainageDensity = NoDataValue;  
  Perimeter_i = vector<int>(1,NoDataValue);
  Perimeter_j =  vector<int>(1,NoDataValue);
  CosmoErosionRate = NoDataValue;
  OtherErosionRate = NoDataValue;
  CHTMean = NoDataValue;
  EStar = NoDataValue;
  RStar = NoDataValue;
   
  //finished creating empty variables 

}

void LSDBasin::create(int JunctionNumber, LSDFlowInfo& FlowInfo, LSDJunctionNetwork& ChanNet)
{

  //NO BOUNDS CHECKING ON JunctionNumber

  //setting all of the instance variables for the given junction

  NRows = ChanNet.get_NRows();
  NCols = ChanNet.get_NCols();
  XMinimum = ChanNet.get_XMinimum();
  YMinimum = ChanNet.get_YMinimum();
  DataResolution = ChanNet.get_DataResolution();
  NoDataValue = ChanNet.get_NoDataValue();
  GeoReferencingStrings = ChanNet.get_GeoReferencingStrings();

  Junction = JunctionNumber;
  
  vector <int> JunctionVector = ChanNet.get_JunctionVector();
  vector <int> ReceiverVector = ChanNet.get_ReceiverVector();
  
  LSDIndexChannel StreamLinkVector = LSDIndexChannel(Junction, JunctionVector[Junction],
                                                     ReceiverVector[Junction], JunctionVector[ReceiverVector[Junction]], FlowInfo);

  int n_nodes_in_channel = StreamLinkVector.get_n_nodes_in_channel();
  int basin_outlet = StreamLinkVector.get_node_in_channel(n_nodes_in_channel-2);
  BasinNodes = FlowInfo.get_upslope_nodes(basin_outlet);
                                                                                     
  NumberOfCells = int(BasinNodes.size());
  Area = NumberOfCells * (DataResolution*DataResolution);
  
  Beheaded = ChanNet.node_tester(FlowInfo, Junction);

  FlowInfo.retrieve_current_row_and_col(ChanNet.get_Node_of_Junction(Junction), Outlet_i, Outlet_j);
    
  vector<int> StreamOrderVector = ChanNet.get_StreamOrderVector();
  
  BasinOrder = StreamOrderVector[Junction];


  int i_max = 0;
  int i_min = 9999999; //a very large number
  int j_max = 0;
  int j_min = 9999999; //a very large number
  
  int i = 0;
  int j = 0;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
    if (i > i_max){i_max = i;}
    else if (i < i_min){i_min = i;}
    if (j > j_max){j_max = j;}
    else if (j < j_min){j_min = j;}
    
  }
  
  Centroid_i = i_min + ((i_max - i_min)/2);
  Centroid_j = j_min + ((j_max - j_min)/2);   //how do these handle 0.5s ??


  //finished setting all the instance variables
  
  
  // now we set up empty variables to store properties of the basin
  // these are populated as they are required using the set methods in LSDBasin
    
  SlopeMean = NoDataValue;
  ElevationMean = NoDataValue;
  AspectMean = NoDataValue;
  ReliefMean = NoDataValue;
  PlanCurvMean = NoDataValue;
  ProfileCurvMean = NoDataValue;
  TotalCurvMean = NoDataValue;
  PlanCurvMax = NoDataValue;
  ProfileCurvMax = NoDataValue;
  TotalCurvMax = NoDataValue;
  HillslopeLength_HFR = NoDataValue;
  HillslopeLength_Binned = NoDataValue;
  HillslopeLength_Spline = NoDataValue;
  HillslopeLength_Density = NoDataValue;
  FlowLength = NoDataValue;
  DrainageDensity = NoDataValue;  
  Perimeter_i = vector<int>(1,NoDataValue);
  Perimeter_j =  vector<int>(1,NoDataValue);
  CosmoErosionRate = NoDataValue;
  OtherErosionRate = NoDataValue;
  CHTMean = NoDataValue;
  EStar = NoDataValue;
  RStar = NoDataValue;
   
  //finished creating empty variables 

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate mean basin value.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinMean(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  float TotalData = 0;
  int CountNDV = 0;
  float BasinAverage;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
    //exclude NDV from average
    if (Data.get_data_element(i,j) != NoDataValue){
      TotalData += Data.get_data_element(i,j);
    }
    else {
      ++CountNDV;
    }
  }

  BasinAverage = TotalData/(NumberOfCells-CountNDV);

  return BasinAverage;
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate max basin value.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinMax(LSDFlowInfo& FlowInfo, LSDRaster Data){

  //could use max_element here? how would that cope with NDVs??

  int i;
  int j;
  float MaxData = -10000000;   //a very small number
  float CurrentData;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    CurrentData = Data.get_data_element(i,j);
    
    //exclude NDV
    if (CurrentData != NoDataValue && CurrentData > MaxData){
      MaxData = CurrentData;     
    }
  }
    
  return MaxData;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate min basin value.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinMin(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  float MinData = 100000000; // a large number
  float CurrentData;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    CurrentData = Data.get_data_element(i,j);
    
    //exclude NDV
    if (CurrentData != NoDataValue && CurrentData < MinData){
      MinData = CurrentData;     
    }
  }
    
  return MinData;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate median basin value.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinMedian(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  vector<float> UnsortedData;
  vector<float> SortedData;
  vector<size_t> index_map;
  float Median;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    //exclude NDV
    if (Data.get_data_element(i,j) != NoDataValue){
      UnsortedData.push_back(Data.get_data_element(i,j));     
    }
  }
  
  //get size of dataset
  size_t n = UnsortedData.size() / 2;
  
  //sort all non NDV values
  matlab_float_sort(UnsortedData, SortedData, index_map);
  
  if (n % 2 != 0){
    Median = (SortedData[n] + SortedData[n+1])/2;
  }
  else{
    Median = SortedData[n];
  }
    
  return Median;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate Standard devaition of the basin values.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinStdDev(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  vector<float> DataValues;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    //exclude NDV
    if (Data.get_data_element(i,j) != NoDataValue){
      DataValues.push_back(Data.get_data_element(i,j));     
    }
  }
  
  float mean = get_mean(DataValues);
  float StdDev = get_standard_deviation(DataValues, mean);
    
  return StdDev;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate Standard Error of the basin values.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinStdError(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  vector<float> DataValues;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    //exclude NDV
    if (Data.get_data_element(i,j) != NoDataValue){
      DataValues.push_back(Data.get_data_element(i,j));     
    }
  }
  
  float mean = get_mean(DataValues);
  float StdDev = get_standard_deviation(DataValues, mean);
  float StdError = get_standard_error(DataValues, StdDev);
    
  return StdError;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate basin range.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
float LSDBasin::CalculateBasinRange(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  float MinData = 100000000; // a large number
  float MaxData = -100000000; // a small number
  float CurrentData;

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    CurrentData = Data.get_data_element(i,j);
    
    //exclude NDV
    if (CurrentData != NoDataValue && CurrentData < MinData){
      MinData = CurrentData;     
    }
    if (CurrentData != NoDataValue && CurrentData > MaxData){
      MaxData = CurrentData;     
    }
  }
  float Range = MaxData - MinData;     
  return Range;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate basin range.
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int LSDBasin::CalculateNumDataPoints(LSDFlowInfo& FlowInfo, LSDRaster Data){

  int i;
  int j;
  int count = 0;
  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
        
    //exclude NDV
    if (Data.get_data_element(i,j) != NoDataValue){
      ++count;     
    }
  }
      
  return count;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate E* and R* values for the basin, using hilltop flow routed hillslope 
// lengths. 
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_EStar_RStar(float CriticalSlope){

    EStar = (2 * (abs(CHTMean)) * HillslopeLength_HFR) / CriticalSlope;
    RStar = ReliefMean / (HillslopeLength_HFR * CriticalSlope);
    
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate flow length for the basin using the D8 flow directions. 
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_FlowLength(LSDIndexRaster& StreamNetwork, LSDFlowInfo& FlowInfo){

  int j;
  int i;
  float LengthSum = 0;
  float two_times_root2 = 2.828427;
  Array2D<int> FlowDir = FlowInfo.get_FlowDirection();


  //Loop over every pixel and record it's stream length and basin ID in two vectors  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
           
     if (StreamNetwork.get_data_element(i,j) != NoDataValue){
       if ((FlowDir[i][j] % 2) != 0 && (FlowDir[i][j] != -1 )){ //is odd but not -1
         LengthSum += (DataResolution * two_times_root2); //diagonal
       }
       else if (FlowDir[i][j] % 2 == 0){  //is even
         LengthSum +=  DataResolution; //cardinal                   
       }
     }
  }

  FlowLength = LengthSum;
 
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Calculate hillslope lengths from boomerang plots. 
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_HillslopeLengths_Boomerang(LSDRaster& Slope, LSDRaster& DinfArea, LSDFlowInfo& FlowInfo, float log_bin_width, int SplineResolution, float bin_threshold){
  
  int j;
  int i;
  Array2D<float> slope(NRows, NCols, NoDataValue);
  Array2D<float> area(NRows, NCols, NoDataValue);
  
  //create subset arrays for just the basin data - this should be rolled into its own method.
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
      slope[i][j] = Slope.get_data_element(i,j);
      area[i][j] = DinfArea.get_data_element(i,j);
    
  }

  //do some log binning
  vector<float> Mean_x_out;
  vector<float> Mean_y_out;
  vector<float> Midpoints_out;
  vector<float> STDDev_x_out;
  vector<float> STDDev_y_out;
  vector<float> STDErr_x_out;
  vector<float> STDErr_y_out;
  vector<int> number_observations;
  
  log_bin_data(area, slope, log_bin_width, Mean_x_out, Mean_y_out, Midpoints_out, STDDev_x_out, STDDev_y_out, STDErr_x_out, STDErr_y_out, number_observations, NoDataValue);  
  
  //remove empty bins 
  RemoveSmallBins(Mean_x_out, Mean_y_out, Midpoints_out, STDDev_x_out, STDDev_y_out, STDErr_x_out, STDErr_y_out, number_observations, bin_threshold);
  
  //index value of max slope
  int slope_max_index = distance(Mean_y_out.begin(), max_element(Mean_y_out.begin(), Mean_y_out.end()));

  //hillslope length from the maximum binned values
  HillslopeLength_Binned = Mean_x_out[slope_max_index]/DataResolution;
      
  // Fit splines through the binned data to get the LH
  vector<float> Spline_X;
  vector<float> Spline_Y;
  PlotCubicSplines(Mean_x_out, Mean_y_out, SplineResolution, Spline_X, Spline_Y);

  //index value of max spline slope
  int slope_max_index_spline = distance(Spline_Y.begin(), max_element(Spline_Y.begin(), Spline_Y.end()));

  //hillslope length from spline curve
  HillslopeLength_Spline = Spline_X[slope_max_index_spline]/DataResolution;

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Generate data to create boomerang plots. 
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::Plot_Boomerang(LSDRaster& Slope, LSDRaster& DinfArea, LSDFlowInfo& FlowInfo, float log_bin_width, int SplineResolution, float bin_threshold, string Path){
  
  int j;
  int i;
  Array2D<float> slope(NRows, NCols, NoDataValue);
  Array2D<float> area(NRows, NCols, NoDataValue);
  
  //create subset arrays for just the basin data - this should be rolled into its own method.
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
      slope[i][j] = Slope.get_data_element(i,j);
      area[i][j] = DinfArea.get_data_element(i,j);
    
  }

  //do some log binning
  vector<float> Mean_x_out;
  vector<float> Mean_y_out;
  vector<float> Midpoints_out;
  vector<float> STDDev_x_out;
  vector<float> STDDev_y_out;
  vector<float> STDErr_x_out;
  vector<float> STDErr_y_out;
  vector<int> number_observations;
  
  log_bin_data(area, slope, log_bin_width, Mean_x_out, Mean_y_out, Midpoints_out, STDDev_x_out, STDDev_y_out, STDErr_x_out, STDErr_y_out, number_observations, NoDataValue);  
  
  //remove empty bins 
  RemoveSmallBins(Mean_x_out, Mean_y_out, Midpoints_out, STDDev_x_out, STDDev_y_out, STDErr_x_out, STDErr_y_out, number_observations, bin_threshold);
        
  // Fit splines through the binned data to get the LH
  vector<float> Spline_X;
  vector<float> Spline_Y;
  PlotCubicSplines(Mean_x_out, Mean_y_out, SplineResolution, Spline_X, Spline_Y);

  //set up a filestream object to write the binned data
  ofstream file;

  stringstream ss_bin;
  ss_bin << Path << Junction << "_boom_binned.txt";
  file.open(ss_bin.str().c_str());   //needs a null terminated character array, not a string. See pg 181 of accelerated c++
    
  for(int q = 0; q < int(Mean_x_out.size()); q++){
    file << Mean_x_out[q] << " " << Mean_y_out[q] << " " << STDDev_x_out[q] << " " << STDDev_y_out[q] << " " << STDErr_x_out[q] << " " << STDErr_y_out[q] << endl;
  }
  file.close();
      
  //set up a filestream object to write the spline data
  ofstream SplineFile;

  stringstream ss_spline;
  ss_spline << Path << Junction << "_boom_spline.txt";
  SplineFile.open(ss_spline.str().c_str());   //needs a null terminated character array, not a string. See pg 181 of accelerated c++
    
  for(int q = 0; q < int(Spline_X.size()); q++){ //fixed bug here where I looped over the wrong vector - SWDG 7/11/13
    SplineFile << Spline_X[q] << " " << Spline_Y[q] << endl;

  }
  SplineFile.close();
  
  //set up a filestream object to write the data cloud
  ofstream cloud;

  stringstream ss_cloud;
  ss_cloud << Path << Junction << "_boom_cloud.txt";
  cloud.open(ss_cloud.str().c_str());     //needs a null terminated character array, not a string. See pg 181 of accelerated c++

  for (int i = 1; i < NRows-1; ++i){
    for (int j = 1; j < NCols-1; ++j){
      if(area[i][j] != NoDataValue && slope[i][j] != NoDataValue){
        cloud << area[i][j] << " " << slope[i][j] << endl;
      }
    }
  }
  cloud.close();

}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Set the mean basin aspect. Does not use the normal basin mean method as angles
// need to be handled differently. 
// Bug fixed in the average calculation when values wrapped around 0
// SWDG 17/2/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_AspectMean(LSDFlowInfo& FlowInfo, LSDRaster Aspect){

  int i;
  int j;
  float avg_r;
  float angle_r;
  float x_component = 0.0;
  float y_component = 0.0;
  int ndv_cell_count = 0;  

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
    if (Aspect.get_data_element(i,j) != NoDataValue){
    
      angle_r = rad(Aspect.get_data_element(i,j));
      x_component += cos(angle_r);
      y_component += sin(angle_r);
  
    }
    else{
      ++ndv_cell_count;
    }
  
  }
    
  avg_r = atan2(y_component, x_component);
  AspectMean = deg(avg_r);
  
  if (AspectMean < 0){
    AspectMean = 360 + AspectMean;
  }
   
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Set the perimeter pixels using a simple edge detection algorithm. This is quite 
// messy and will be improved soon.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_Perimeter(LSDFlowInfo& FlowInfo){

  int i;
  int j;
  vector<int> I;
  vector<int> J;
  int NDVCount = 0;
  Array2D<float> BasinData(NRows, NCols, NoDataValue);

  //create subset arrays for just the basin data - this should be rolled into its own method.
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
      BasinData[i][j] = BasinNodes[q];
    
  }

  for (int q = 0; q < int(BasinNodes.size()); ++q){
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
      NDVCount = 0;
     
        //count border cells that are NDV
        if (BasinData[i-1][j-1] == NoDataValue){ ++NDVCount; }
        if (BasinData[i][j-1] == NoDataValue){ ++NDVCount; }
        if (BasinData[i+1][j-1] == NoDataValue){ ++NDVCount; }
        if (BasinData[i-1][j] == NoDataValue){ ++NDVCount; }
        if (BasinData[i+1][j] == NoDataValue){ ++NDVCount; }
        if (BasinData[i-1][j+1] == NoDataValue){ ++NDVCount; }
        if (BasinData[i][j+1] == NoDataValue){ ++NDVCount; }
        if (BasinData[i+1][j+1] == NoDataValue){ ++NDVCount; }
        
        if (NDVCount >= 4 && NDVCount < 8){  //increase the first value to get a simpler polygon
          //edge pixel
          I.push_back(i);
          J.push_back(j);
        }
    
  }

  //now have 2 vectors of i and j indexes of every point
  Perimeter_i = I;
  Perimeter_j = J;


}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Set the four different hillslope length measurements for the basin. 
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_all_HillslopeLengths(LSDFlowInfo& FlowInfo, LSDRaster& HillslopeLengths, LSDRaster& Slope, LSDRaster& DinfArea, float log_bin_width, int SplineResolution, float bin_threshold){

  set_HillslopeLength_HFR(FlowInfo, HillslopeLengths);
  set_HillslopeLengths_Boomerang(Slope, DinfArea, FlowInfo, log_bin_width, SplineResolution, bin_threshold);

  if (DrainageDensity != NoDataValue){ 
    set_HillslopeLength_Density();
  }
  else{
    cout << "\nDrainage Density has not been set, so the hillslope length cannot be set." << endl;
  }

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Set all of the basin parameters with one call.
//
// Runs polyfit to get the elevation derivatives, so can be quite memory intensive. Method
// calls all the setters one by one, to populate all the basin parameters. So a
// basin can be created and all it's properties set with 2 calls. The erosion rates have default 
// parameters of -9999 as these are rarely used variables.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDBasin::set_All_Parameters(LSDRaster& Elevation, LSDFlowInfo& FlowInfo, LSDRaster& CHT, LSDIndexRaster& StreamNetwork,
                                  LSDRaster& HillslopeLengths, LSDRaster& Relief, float window_radius, float log_bin_width,
                                  int SplineResolution, float bin_threshold, float CriticalSlope, float CosmoErosionRate, 
                                  float OtherErosionRate){

  // coefficent matrices for polyfit routine
  Array2D<float> a;
  Array2D<float> b;
  Array2D<float> c;
  Array2D<float> d;
  Array2D<float> e;
  Array2D<float> f;

  Elevation.calculate_polyfit_coefficient_matrices(window_radius, a, b, c, d, e, f);
  LSDRaster TotalCurv = Elevation.calculate_polyfit_curvature (a,b);
  LSDRaster ProfileCurv = Elevation.calculate_polyfit_profile_curvature (a,b,c,d,e);
  LSDRaster PlanCurv = Elevation.calculate_polyfit_planform_curvature (a,b,c,d,e);
  LSDRaster Aspect = Elevation.calculate_polyfit_aspect(d,e);  
  LSDRaster Slope = Elevation.calculate_polyfit_slope(d,e);
  LSDRaster DinfArea = Elevation.D_inf_units(); 
  
  set_SlopeMean(FlowInfo, Slope);
  set_ElevationMean(FlowInfo, Elevation);
  set_ReliefMean(FlowInfo, Relief);
  set_PlanCurvMean(FlowInfo, PlanCurv);
  set_ProfileCurvMean(FlowInfo, ProfileCurv);
  set_TotalCurvMean(FlowInfo, TotalCurv);
  set_PlanCurvMax(FlowInfo, PlanCurv);
  set_ProfileCurvMax(FlowInfo, ProfileCurv);
  set_TotalCurvMax(FlowInfo, TotalCurv);
  set_CHTMean(FlowInfo, CHT);
  set_AspectMean(FlowInfo, Aspect);
  set_FlowLength(StreamNetwork, FlowInfo);
  set_DrainageDensity();
  set_all_HillslopeLengths(FlowInfo, HillslopeLengths, Slope, DinfArea, log_bin_width, SplineResolution, bin_threshold);
  set_Perimeter(FlowInfo);
  set_EStar_RStar(CriticalSlope);
  set_CosmoErosionRate(CosmoErosionRate);
  set_OtherErosionRate(OtherErosionRate);

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Write integer basin parameters into the shape of the basin.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
LSDIndexRaster LSDBasin::write_integer_data_to_LSDIndexRaster(int Param, LSDFlowInfo FlowInfo){
  
  int i;
  int j; 
  Array2D<int> Output(NRows, NCols, NoDataValue);
  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    Output[i][j] = Param;
  }

  LSDIndexRaster OutputRaster(NRows, NCols, XMinimum, YMinimum, DataResolution,
                               NoDataValue, Output, GeoReferencingStrings);

  return OutputRaster;

}                                       

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Write real basin parameters into the shape of the basin.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
LSDRaster LSDBasin::write_real_data_to_LSDRaster(float Param, LSDFlowInfo FlowInfo){
  
  int i;
  int j; 
  Array2D<float> Output(NRows, NCols, NoDataValue);
  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    Output[i][j] = Param;
  }

  LSDRaster OutputRaster(NRows, NCols, XMinimum, YMinimum, DataResolution,
                               NoDataValue, Output, GeoReferencingStrings);

  return OutputRaster;

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Cookie cut data from an LSDRaster into the shape of the basin.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
LSDRaster LSDBasin::write_raster_data_to_LSDRaster(LSDRaster Data, LSDFlowInfo FlowInfo){
  
  int i;
  int j; 
  Array2D<float> Output(NRows, NCols, NoDataValue);
  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    Output[i][j] = Data.get_data_element(i,j);
  }

  LSDRaster OutputRaster(NRows, NCols, XMinimum, YMinimum, DataResolution,
                               NoDataValue, Output, GeoReferencingStrings);

  return OutputRaster;

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Cookie cut data from an LSDIndexRaster into the shape of the basin.
// SWDG 12/12/13
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
LSDIndexRaster LSDBasin::write_raster_data_to_LSDIndexRaster(LSDIndexRaster Data, LSDFlowInfo FlowInfo){
  
  int i;
  int j; 
  Array2D<int> Output(NRows, NCols, NoDataValue);
  
  for (int q = 0; q < int(BasinNodes.size()); ++q){
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    Output[i][j] = Data.get_data_element(i,j);
  }

  LSDIndexRaster OutputRaster(NRows, NCols, XMinimum, YMinimum, DataResolution,
                               NoDataValue, Output, GeoReferencingStrings);

  return OutputRaster;

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Function to check whether a node is in the basin
// FJC 21/02/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int LSDBasin::is_node_in_basin(int test_node)
{
  int node_checker = 0;
  for (int i =0; i < int(BasinNodes.size()); i++)
  {
    if (test_node == BasinNodes[i])
    {
      node_checker = 1;
    }
  }  
  return node_checker;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Method to merge a vector of LSDRaster basins generated using LSDBasin into
// a single LSDRaster for visualisation.
//
// 50% less computationally expesnive than the old method, but still very 
// inefficient. Does not test for overlaps in the data, will simply overwrite
// so that the last value to occupy a cell will be written. 
//
// SWDG 07/12/14
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
LSDRaster LSDBasin::Merge_Basins(vector<LSDRaster> Basins)
{
  
  Array2D<float> Output(NRows,NCols,NoDataValue);
  
  for (int q = 0; q < int(Basins.size()); ++q){
  
    for (int i = 0; i < NRows; ++i){
      for (int j = 0; j < NCols; ++j){
        if (Basins[q].get_data_element(i,j) != NoDataValue){
          Output[i][j] = Basins[q].get_data_element(i,j);
        }      
      }      
    }
                                 
  }
  
  LSDRaster OutputRaster(NRows, NCols, XMinimum, YMinimum, DataResolution,
                               NoDataValue, Output, GeoReferencingStrings);
                               
  return OutputRaster;
}



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  +++++++++++++++++
//  +++++++++++++++++
//  +++++++++++++++++
//  +++++++++++++++++
//  +++++++++++++++++
//  COSMOGENIC BASIN
//  +++++++++++++++++
//  +++++++++++++++++
//  +++++++++++++++++
//  +++++++++++++++++
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This creates a cosmogenic basin. Similar to a normal basin but just has the 
// 10Be and 26Al concentrations
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::create(int JunctionNumber, LSDFlowInfo& FlowInfo, 
                           LSDJunctionNetwork& ChanNet, 
                           double N10Be, double delN10Be, 
                           double N26Al, double delN26Al)
{

  // The measured 10Be concentration    
  measured_N_10Be = N10Be;
    
  // The measured 26Al concentration
  measured_N_26Al = N26Al;
    
  // The measured uncertainty in the 10Be concentration
  delN_10Be = delN10Be;
    
  // The measured uncertainty in the 10Be concentration
  delN_26Al = delN26Al;

  //NO BOUNDS CHECKING ON JunctionNumber

  //setting all of the instance variables for the given junction

  NRows = ChanNet.get_NRows();
  NCols = ChanNet.get_NCols();
  XMinimum = ChanNet.get_XMinimum();
  YMinimum = ChanNet.get_YMinimum();
  DataResolution = ChanNet.get_DataResolution();
  NoDataValue = ChanNet.get_NoDataValue();
  GeoReferencingStrings = ChanNet.get_GeoReferencingStrings();

  Junction = JunctionNumber;
  
  //cout << "LSDCosmoBasin, line 888, Junction is: " << Junction << endl; 
  
  int basin_outlet = ChanNet.get_Node_of_Junction(Junction);
  BasinNodes = FlowInfo.get_upslope_nodes(basin_outlet);
  
  //cout << "LSDCosmoBasin, Line 893, basin outlet is: " << basin_outlet << endl;
                                                                                     
  NumberOfCells = int(BasinNodes.size());
  Area = NumberOfCells * (DataResolution*DataResolution);
  
  Beheaded = ChanNet.node_tester(FlowInfo, Junction);

  FlowInfo.retrieve_current_row_and_col(ChanNet.get_Node_of_Junction(Junction), Outlet_i, Outlet_j);
    
  vector<int> StreamOrderVector = ChanNet.get_StreamOrderVector();
  
  BasinOrder = StreamOrderVector[Junction];


  int i_max = 0;
  int i_min = 9999999; //a very large number
  int j_max = 0;
  int j_min = 9999999; //a very large number
  
  int i = 0;
  int j = 0;

  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], i, j);
    
    if (i > i_max){i_max = i;}
    else if (i < i_min){i_min = i;}
    if (j > j_max){j_max = j;}
    else if (j < j_min){j_min = j;}
    
  }
  
  Centroid_i = i_min + ((i_max - i_min)/2);
  Centroid_j = j_min + ((j_max - j_min)/2);   //how do these handle 0.5s ??


  //finished setting all the instance variables


}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  This function populates the topographic and production shielding
// It sets the snow sheilding to a default of 1
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_scaling_vectors(LSDFlowInfo& FlowInfo, 
                                               LSDRaster& Elevation_Data,
                                               LSDRaster& T_Shield,
                                               string path_to_atmospheric_data)
{
  int row,col;
  
  // variables for converting location and elevation
  double this_elevation, this_pressure, this_tshield;

  vector<double> tshield_temp;
  vector<double> prod_temp;
  vector<double> snow_temp;
  
  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  
  // get the atmospheric parameters
  LSDCRNP.load_parameters_for_atmospheric_scaling(path_to_atmospheric_data);
  LSDCRNP.set_CRONUS_data_maps();
  
  // a function for scaling stone production, defaults to 1
  double Fsp = 1.0;
  
  // the latitude and longitude
  double lat,longitude;
  
  // decalre converter object
  LSDCoordinateConverterLLandUTM Converter;
  
  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
    //exclude NDV from average
    if (Elevation_Data.get_data_element(row,col) != NoDataValue)
    {
      // To get pressure, first get the lat and long
      Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude, Converter);
      //Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude);
      
      // now the elevation
      this_elevation = Elevation_Data.get_data_element(row,col);
      
      // now the pressure
      this_pressure = LSDCRNP.NCEPatm_2(double(lat), double(longitude), 
                                        double(this_elevation));

      //cout << "r: " << row << " c: " << col << " lat: " << lat << " long: " << longitude
      //     << " elevation: " << this_elevation << " pressure: " << this_pressure << endl;
           
      
      // now get the scaling
      prod_temp.push_back(LSDCRNP.stone2000sp(lat,this_pressure, Fsp));
      
      // Now get topographic shielding
      this_tshield = double(T_Shield.get_data_element(row,col));
      tshield_temp.push_back(this_tshield);
      
      // now set the snow sheilding to 1
      snow_temp.push_back(1.0);
      
    }
    else 
    {
      prod_temp.push_back(double(NoDataValue));
      tshield_temp.push_back(double(NoDataValue));
      snow_temp.push_back(double(NoDataValue));
    }
  }

  // set the shielding vectors
  topographic_shielding = tshield_temp;
  production_scaling =  prod_temp;
  snow_shielding = snow_temp;
  
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  This function populates the topographic and production shielding
//  It loads a snow shielding raster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_scaling_vectors(LSDFlowInfo& FlowInfo, 
                                               LSDRaster& Elevation_Data,
                                               LSDRaster& T_Shield,
                                               LSDRaster& S_Shield,
                                               string path_to_atmospheric_data)
{
  int row,col;
  
  // variables for converting location and elevation
  double this_elevation, this_pressure, this_tshield, this_sshield;

  vector<double> tshield_temp;
  vector<double> prod_temp;
  vector<double> snow_temp;
  
  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  
  // get the atmospheric parameters
  LSDCRNP.load_parameters_for_atmospheric_scaling(path_to_atmospheric_data);
  LSDCRNP.set_CRONUS_data_maps();
  
  // a function for scaling stone production, defaults to 1
  double Fsp = 1.0;
  
  // the latitude and longitude
  double lat,longitude;
  
  // decalre converter object
  LSDCoordinateConverterLLandUTM Converter;
  
  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
    //exclude NDV from average
    if (Elevation_Data.get_data_element(row,col) != NoDataValue)
    {
      // To get pressure, first get the lat and long
      Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude, Converter);
      //Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude);
      
      // now the elevation
      this_elevation = Elevation_Data.get_data_element(row,col);
      
      // now the pressure
      this_pressure = LSDCRNP.NCEPatm_2(double(lat), double(longitude), 
                                        double(this_elevation));

      //cout << "r: " << row << " c: " << col << " lat: " << lat << " long: " << longitude
      //     << " elevation: " << this_elevation << " pressure: " << this_pressure << endl;
           
      
      // now get the scaling
      prod_temp.push_back(LSDCRNP.stone2000sp(lat,this_pressure, Fsp));
      
      // Now get topographic shielding
      this_tshield = double(T_Shield.get_data_element(row,col));
      tshield_temp.push_back(this_tshield);
      
      // now get the snow sheilding
      this_sshield = double(S_Shield.get_data_element(row,col));
      snow_temp.push_back(this_sshield);
      
    }
    else 
    {
      prod_temp.push_back(double(NoDataValue));
      tshield_temp.push_back(double(NoDataValue));
      snow_temp.push_back(double(NoDataValue));
    }
  }

  // set the shielding vectors
  topographic_shielding = tshield_temp;
  production_scaling =  prod_temp;
  snow_shielding = snow_temp;
  
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function poplates the atmospheric pressure vector. 
// It is used for bug-checking and comparison with other cosmo calculators
// Other function do not require this to be called
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::get_atmospheric_pressure(LSDFlowInfo& FlowInfo, 
                    LSDRaster& Elevation_Data, string path_to_atmospheric_data)
{
  int row,col;
  
  // variables for converting location and elevation
  double this_elevation, this_pressure;

  vector<double> pressure_temp;

  
  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  
  // get the atmospheric parameters
  LSDCRNP.load_parameters_for_atmospheric_scaling(path_to_atmospheric_data);
  LSDCRNP.set_CRONUS_data_maps();
  
  // the latitude and longitude
  double lat,longitude;
  
  // decalre converter object
  LSDCoordinateConverterLLandUTM Converter;
  
  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
    //exclude NDV from average
    if (Elevation_Data.get_data_element(row,col) != NoDataValue)
    {
      // To get pressure, first get the lat and long
      Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude, Converter);
      //Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude);
      
      // now the elevation
      this_elevation = Elevation_Data.get_data_element(row,col);
      
      // now the pressure
      this_pressure = LSDCRNP.NCEPatm_2(double(lat), double(longitude), 
                                        double(this_elevation));

      //cout << "r: " << row << " c: " << col << " lat: " << lat << " long: " << longitude
      //     << " elevation: " << this_elevation << " pressure: " << this_pressure << endl;
           
      
      // now get the scaling
      pressure_temp.push_back(this_pressure);
      
    }
    else 
    {
      pressure_temp.push_back(double(NoDataValue));
    }
  }

  // set the pressure vector
  atmospheric_pressure = pressure_temp;
  
  // print pressure at outlet
  cout << "the pressure at the outlet is: "  << atmospheric_pressure[0] << " mbar" << endl;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function creates the snow and shelf sheilding vectors based on two 
// rasters
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_snow_and_self_eff_depth_vectors(LSDFlowInfo& FlowInfo, 
                                              LSDRaster& snow_eff_depth, 
                                              LSDRaster& self_eff_depth)
{
  int row,col;
  
  // the effective depths at individual nodes
  double this_eff_snow_depth;
  double this_eff_self_depth;

  // temporary vectors that will be copied into the 
  vector<double> snow_temp;
  vector<double> self_temp;

  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    //exclude NDV from average
    if (snow_eff_depth.get_data_element(row,col) != NoDataValue)
    {
      // get the row and column of the node
      FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
      // get the snow and self shielding
      this_eff_snow_depth = double(snow_eff_depth.get_data_element(row,col));
      this_eff_self_depth = double(self_eff_depth.get_data_element(row,col));
    
      // add data to the vectors
      snow_temp.push_back(this_eff_snow_depth);
      self_temp.push_back(this_eff_self_depth);
    }
  }
  
  // update the vectors in the basin object
  self_shield_eff_depth = self_temp;
  snow_shield_eff_depth = snow_temp;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function creates the snow and shelf sheilding vectors based on a 
// double and a float
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_snow_and_self_eff_depth_vectors(LSDFlowInfo& FlowInfo, 
                                              double snow_eff_depth, 
                                              LSDRaster& self_eff_depth)
{
  int row,col;
  
  // the effective depths at individual nodes
  double this_eff_self_depth;

  // temporary vectors that will be copied into the 
  vector<double> snow_temp;
  vector<double> self_temp;
  
  // first put the one element in the snow temp vector
  snow_temp.push_back(snow_eff_depth);

  // now loop through the other vector adding elements
  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    //exclude NDV from average
    if (self_eff_depth.get_data_element(row,col) != NoDataValue)
    {
      // get the row and column of the node
      FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
      // get the snow and self shielding
      this_eff_self_depth = double(self_eff_depth.get_data_element(row,col));
    
      // add data to the vectors
      self_temp.push_back(this_eff_self_depth);
    }
  }
  
  // update the vectors in the basin object
  self_shield_eff_depth = self_temp;
  snow_shield_eff_depth = snow_temp;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function creates the snow and shelf sheilding vectors based on a 
// double and a float
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_snow_and_self_eff_depth_vectors(LSDFlowInfo& FlowInfo, 
                                              LSDRaster& snow_eff_depth, 
                                              double self_eff_depth)
{
  int row,col;
  
  // the effective depths at individual nodes
  double this_eff_snow_depth;

  // temporary vectors that will be copied into the 
  vector<double> snow_temp;
  vector<double> self_temp;
  
  // first put the one element in the self temp vector
  self_temp.push_back(self_eff_depth);

  // now loop through the other vector adding elements
  for (int q = 0; q < int(BasinNodes.size()); ++q)
  {
    
    //exclude NDV from average
    if (snow_eff_depth.get_data_element(row,col) != NoDataValue)
    {
      // get the row and column of the node
      FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
    
      // get the snow and self shielding
      this_eff_snow_depth = double(snow_eff_depth.get_data_element(row,col));
    
      // add data to the vectors
      snow_temp.push_back(this_eff_snow_depth);
    }
  }
  
  // update the vectors in the basin object
  self_shield_eff_depth = self_temp;
  snow_shield_eff_depth = snow_temp;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function creates the snow and shelf sheilding vectors based on a 
// double and a float
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::populate_snow_and_self_eff_depth_vectors(double snow_eff_depth, 
                                                             double self_eff_depth)
{
  // temporary vectors that will be copied into the 
  vector<double> snow_temp;
  vector<double> self_temp;
  
  // first put the one element in the snow temp vector
  self_temp.push_back(self_eff_depth);
  snow_temp.push_back(snow_eff_depth);
  
  // update the vectors in the basin object
  self_shield_eff_depth = self_temp;
  snow_shield_eff_depth = snow_temp;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This resets the snow and self shielding vectors
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::reset_snow_and_self_eff_depth_vectors()
{
  // temporary vectors that will be copied into the 
  vector<double> snow_temp;
  vector<double> self_temp;
  
  // update the vectors in the basin object
  self_shield_eff_depth = self_temp;
  snow_shield_eff_depth = snow_temp;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// This function wraps the erosion rate calculations with formal error analysis
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- 
vector<double> LSDCosmoBasin::full_CRN_erosion_analysis(double Nuclide_conc, string Nuclide, 
                              double Nuclide_conc_err, double prod_uncert_factor,
                              string Muon_scaling)
{
  // the vector for holding the erosion rates and uncertainties
  vector<double> erate_uncert_vec;
  
  double erate;                   // effective erosion rate g/cm^2/yr
  double erate_external_plus;     // effective erosion rate g/cm^2/yr for AMS uncertainty +
  double erate_external_minus;    // effective erosion rate g/cm^2/yr for AMS uncertainty -
  double dEdExternal;             // change in erosion rate for change in AMS atoms/g
  double External_uncert;         // uncertainty of effective erosion rate g/cm^2/yr for AMS
  
  
  double production_uncertainty;  // a lumped production uncertainty value. 
                                  // not generally used but needs to be passed
                                  // to the erosion finding routines as a parameter
  
  // variables for the muon uncertainty
  double average_production_rate; // The average production rate, used in uncertainty
                                  // calculations
  double erate_muon_scheme_schaller;  // erosion rate using schaller scheme
  double erate_muon_scheme_braucher;  // erosion rate using braucher scheme
  
  double dEdMuonScheme;           // change in erosion rate for change in Muon Scheme
  double Muon_uncert;             // uncertainty of effective erosion rate 
                                  // in g/cm^2/yr for different muon schemes
  
  // variable for the production uncertainty
  double erate_prod_plus;   // erosion rate for positive production uncertainty
  double erate_prod_minus;  // erosion rate for negative production uncertainty
  
  double dEdProduction;        // change in erosion rate for change in production
  double Prod_uncert;          // uncertainty of effective erosion rate 
                               // in g/cm^2/yr for production uncertainty
  
  double this_prod_difference; // the difference in production for production uncertainty
  
  // initially we do not modify production rates
  bool is_production_uncertainty_plus_on = false;
  bool is_production_uncertainty_minus_on = false;
  
  // first get the prediction of the erosion rate
  erate = predict_CRN_erosion(Nuclide_conc, Nuclide, prod_uncert_factor, 
                              Muon_scaling, production_uncertainty,
                              average_production_rate,
                              is_production_uncertainty_plus_on,
                              is_production_uncertainty_minus_on);
  
  double no_prod_uncert = 1.0;    // set the scheme to no production uncertainty
                                  // for the external uncertainty
  // now get the external uncertainty                                        
  erate_external_plus = predict_CRN_erosion(Nuclide_conc+Nuclide_conc_err, Nuclide, 
                                             no_prod_uncert, Muon_scaling,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on);
  erate_external_minus = predict_CRN_erosion(Nuclide_conc-Nuclide_conc_err, Nuclide, 
                                             no_prod_uncert, Muon_scaling,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on); 
  dEdExternal = (erate_external_plus-erate_external_minus)/(2*Nuclide_conc_err);
  External_uncert = fabs(dEdExternal*Nuclide_conc_err);
  
  //cout << "LSDCosmoBasin, line 1160, erate: " << erate << " and uncertainty: " 
  //     << External_uncert << endl;

  // now calculate uncertainty from different muon scaling schemes. 
  // The end members are Braucher and Schaller
  string braucher_string = "Braucher";
  string schaller_string = "Schaller";
  
  // get the difference in the pair
  LSDCRNParameters LSDCRNP;
  int pair_key = 0;       // this is for braucher-schaller
  vector<double> muon_uncert_diff = LSDCRNP.get_uncertainty_scaling_pair(pair_key);
  
  double this_muon_uncert_dif;
  if(Nuclide == "Be10")
  {
    this_muon_uncert_dif = muon_uncert_diff[0];
  }
  else if (Nuclide == "Al26")
  {
    this_muon_uncert_dif = muon_uncert_diff[1];
  }
  else
  {
    cout << "LINE 1295 LSDBasin you did not supply a valid nuclide, defaulting to 10Be" << endl;
    Nuclide = "Be10";
    this_muon_uncert_dif = muon_uncert_diff[0];
  }
  
  // now get the muon uncertainty
  erate_muon_scheme_schaller = predict_CRN_erosion(Nuclide_conc, Nuclide, 
                                             no_prod_uncert, schaller_string,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on);
  erate_muon_scheme_braucher = predict_CRN_erosion(Nuclide_conc, Nuclide, 
                                             no_prod_uncert, braucher_string,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on);
  dEdMuonScheme = (erate_muon_scheme_schaller-erate_muon_scheme_braucher)/
                  this_muon_uncert_dif;
  Muon_uncert = fabs(dEdMuonScheme*this_muon_uncert_dif);
  
  //cout << "LSDCosmoBasin, Line 1292, change in scaling production rate: " 
  //     << this_muon_uncert_dif << " erate Schal: "
  //     << erate_muon_scheme_schaller << " erate Braucher: " 
  //     << erate_muon_scheme_braucher << " and erate uncert: " << Muon_uncert << endl;
  
  // now get the production uncertainty
  // first set the scaling
  // reset scaling parameters. This is necessary since the F values are
  // reset for local scaling
  if (Muon_scaling == "Schaller" )
  {
    LSDCRNP.set_Schaller_parameters();
  }
  else if (Muon_scaling == "Braucher" )
  {
    LSDCRNP.set_Braucher_parameters();
  }
  else if (Muon_scaling == "Granger" )
  {
    LSDCRNP.set_Granger_parameters();
  }
  else
  {
    cout << "You didn't set the muon scaling." << endl
         << "Options are Schaller, Braucher and Granger." << endl
         << "You chose: " << Muon_scaling << endl
         << "Defaulting to Braucher et al (2009) scaling" << endl;
    LSDCRNP.set_Braucher_parameters();     
  }
  
  // now get the uncertainty parameters
  vector<double> prod;
  double prod_plus,prod_minus;
  // get the nuclide concentration from this node
  if (Nuclide == "Be10")
  {
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
    prod_plus = prod[0];
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
    prod_minus = prod[0];
  }
  else if (Nuclide == "Al26")
  {
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
    prod_plus = prod[1];
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
    prod_minus = prod[1];
  }
  else
  {
    cout << "You didn't give a valid nuclide. You chose: " << Nuclide << endl;
    cout << "Choices are 10Be, 26Al.  Note these case sensitive and cannot" << endl;
    cout << "contain spaces or control characters. Defaulting to 10Be." << endl;
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
    prod_plus = prod[0];
    prod = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
    prod_minus = prod[0];
  }
  //cout << "Prod plus: " << prod_plus << " prod minus: " << prod_minus << endl;
  this_prod_difference = prod_plus+prod_minus;     
  
  
  is_production_uncertainty_plus_on = true;
  is_production_uncertainty_minus_on = false;
  erate_prod_plus = predict_CRN_erosion(Nuclide_conc, Nuclide, 
                                             no_prod_uncert, schaller_string,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on);

  is_production_uncertainty_plus_on = false;
  is_production_uncertainty_minus_on = true;
  erate_prod_minus = predict_CRN_erosion(Nuclide_conc, Nuclide, 
                                             no_prod_uncert, schaller_string,
                                             production_uncertainty,
                                             average_production_rate,
                                             is_production_uncertainty_plus_on,
                                             is_production_uncertainty_minus_on);  
  
  dEdProduction = (erate_prod_plus-erate_prod_minus)/
                   this_prod_difference;
  Prod_uncert = fabs(dEdProduction*this_prod_difference);

  //cout << "LSDCosmoBasin, Line 1368, change in production rate for production uncertainty: " 
  //     << this_prod_difference << " erate plus: "
  //     << erate_prod_plus << " erate minus: " 
  //     << erate_prod_minus << " and erate uncert: " << Prod_uncert << endl;



  // now calculate the total uncertainty
  double total_uncert = sqrt( External_uncert*External_uncert +
                              Muon_uncert*Muon_uncert +
                              Prod_uncert*Prod_uncert);

  erate_uncert_vec.push_back(erate);
  erate_uncert_vec.push_back(External_uncert);
  erate_uncert_vec.push_back(Muon_uncert);
  erate_uncert_vec.push_back(Prod_uncert);
  erate_uncert_vec.push_back(total_uncert);
  
  return erate_uncert_vec;
} 




//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// This function is used to get the erosion rate using Newton-Raphson
// method of root finding
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- 
double LSDCosmoBasin::predict_CRN_erosion(double Nuclide_conc, string Nuclide, 
                                          double prod_uncert_factor,
                                          string Muon_scaling,
                                          double& production_uncertainty,
                                          double& average_production,
                                          bool is_production_uncertainty_plus_on,
                                          bool is_production_uncertainty_minus_on)
{
  // effective erosion rates (in g/cm^2/yr) for running the Newton Raphson
  // iterations
  double erate_guess;
  double eff_erate_guess;
  //double this_eff_erosion_rate;
  //double d_eff_erosion_rate;
  
  double rho = 2650;  // this is the rock density but in fact it doesn't 
                      // really play a role since it is factored into the
                      // apparent erosion to get erosion in mm/yr but the divided
                      // out again. 
                      // The value 2650 is used because this is the default in cosmocalc

  // production uncertainty factor is a multiplier that sets the production 
  // certainty. If it is 1.1, there is 10% production rate uncertainty, or
  // if it is 0.9 there is -10% unvertainty. The reason why it is implemented
  // like this is that this allows gaussian error propigation.
  if (prod_uncert_factor <=0)
  {
    cout << "You have set an unrealistic production uncertainty factor." << endl;
    cout << "Defaulting to 1." << endl;
    prod_uncert_factor = 1;
  }

  // initiate a particle. We'll just repeatedly call this particle
  // for the sample. 
  int startType = 0; 
  double Xloc = 0;
  double Yloc = 0;
  double  startdLoc = 0.0;
  double  start_effdloc = 0.0;
  double startzLoc = 0.0;
  // create a particle at zero depth
  LSDCRNParticle eroded_particle(startType, Xloc, Yloc,
                               startdLoc, start_effdloc, startzLoc);

  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;

  // check the production uncertainty bools
  if(is_production_uncertainty_plus_on)
  {
    if(is_production_uncertainty_minus_on)
    {
      cout << "You can't have both plus and minus production uncertainty on" << endl;
      cout << "Setting minus uncertainty to false" << endl;
      is_production_uncertainty_minus_on = false;
    }
  }

  
  // now set the scaling parameters
  if (Muon_scaling == "Schaller" )
  {
    LSDCRNP.set_Schaller_parameters();
  }
  else if (Muon_scaling == "Braucher" )
  {
    LSDCRNP.set_Braucher_parameters();
  }
  else if (Muon_scaling == "Granger" )
  {
    LSDCRNP.set_Granger_parameters();
  }
  else
  {
    cout << "You didn't set the muon scaling." << endl
         << "Options are Schaller, Braucher and Granger." << endl
         << "You chose: " << Muon_scaling << endl
         << "Defaulting to Braucher et al (2009) scaling" << endl;
    LSDCRNP.set_Braucher_parameters();     
  }

  // set the scaling vector
  vector<bool> nuclide_scaling_switches(4,false);
  if (Nuclide == "Be10")
  {
    nuclide_scaling_switches[0] = true;
  }
  else if (Nuclide == "Al26")
  {
    nuclide_scaling_switches[1] = true;
  }
  else
  {
    cout << "LSDBasin line 1583, You didn't choos a valid nuclide. Defaulting"
         << " to 10Be." << endl;
    Nuclide = "Be10";
    nuclide_scaling_switches[0] = true; 
  }

  // now get the guess from the particle
  // the initial guess just takes scaling from the outlet, and then 
  // uses that for the entire basin. This guess will probably be quite
  // far off, but provides a useful starting point
  
  // the elevation, snow shielding, topographic shielding
  // and production scaling are all independent of the erosion rate
  // and are calculated seperately. 
  // IMPORTANT populate scaling vectors must be called in advance!
  if(  production_scaling.size() < 1 )
  {
    cout << "LSDCosmoBasin, trying to precalculate erosion rate." << endl
         << "Scaling vectors have not been set! You are about to get a seg fault" << endl;
  }
  double total_shielding =  production_scaling[0]*topographic_shielding[0]*
                        snow_shielding[0];
                        
  //cout << "LSDBasin line 1128 Prod scaling is: " << production_scaling[0] << endl;
                        
  //cout << "LSDBasin line 1129; total scaling is: " << total_shielding << endl;
                      
  // now recalculate F values to match the total shielding
  LSDCRNP.scale_F_values(total_shielding,nuclide_scaling_switches);
  LSDCRNP.set_neutron_scaling(production_scaling[0],topographic_shielding[0],
                             snow_shielding[0]);
  
  // at the moment do only the outlet
  bool data_from_outlet_only = false;
  //cout << "LSDBasin line 1739 WARNING YOU ARE ONLY CALCULATING THE OUTLET " << endl;
  
  // get the nuclide concentration from this node
  if (Nuclide == "Be10")
  {
    //cout << "LINE 1134 LSDBasin Nuclide conc is: " << Nuclide_conc << endl;
    eroded_particle.setConc_10Be(Nuclide_conc);
    erate_guess = eroded_particle.apparent_erosion_10Be_neutron_only(rho, LSDCRNP);
    //cout << "Be10, initial erate guess in m/yr with density " << rho << ": " << erate_guess << endl;
  }
  else if (Nuclide == "Al26")
  {
    eroded_particle.setConc_26Al(Nuclide_conc);
    erate_guess = eroded_particle.apparent_erosion_26Al_neutron_only(rho, LSDCRNP);
  }
  else
  {
    cout << "You didn't give a valid nuclide. You chose: " << Nuclide << endl;
    cout << "Choices are 10Be, 26Al.  Note these case sensitive and cannot" << endl;
    cout << "contain spaces or control characters. Defaulting to 10Be." << endl;
    eroded_particle.setConc_10Be(Nuclide_conc);
    erate_guess = eroded_particle.apparent_erosion_10Be_neutron_only(rho, LSDCRNP);
  }
  
  // convert to  g/cm^2/yr
  eff_erate_guess = 0.1*erate_guess*rho;
  
  // now using this as the initial guess, use Newton-Raphson to zero in on the
  // correct erosion rate
  double eff_e_new = eff_erate_guess; // the erosion rate upon which we iterate
  double eff_e_change;                // the change in erosion rate between iterations
  double tolerance = 1e-10;           // tolerance for a change in the erosion rate
                                      // between Newton-Raphson iterations
  double eff_e_displace = 1e-6;       // A small displacment in the erosion rate used
                                      // to calculate the derivative
  double N_this_step;                 // the concentration of the nuclide reported this step
  double N_displace;                  // the concentration at the displaced erosion rate
  double N_derivative;                // dN/de derivative for Newton-Raphson
  double f_x;                         // the function being tested by newton raphson
  double f_x_displace;                // the displaced function (for calculating the derivative)
  
  double this_step_prod_uncert;       // the uncertainty in the production rate
                                      // from this step
  double displace_uncertainty;        // the uncertainty from the displaced calculations
                                      // is not used so a dummy variable is used here
  
  double this_step_average_production;// the average production rate for this step
  double displace_average_production; // aveage production for the displace step
  
  do
  {
    // get the new values
    //cout << "Taking a step, eff_e: " << eff_e_new << " data_outlet? " <<  data_from_outlet_only;
    if(self_shield_eff_depth.size() < 1 && snow_shield_eff_depth.size() < 1)
    {                                             
      //cout << "LSDBasin line 1630, You are doing this wihout the effective depth driven shielding" << endl;
      
      N_this_step = predict_mean_CRN_conc(eff_e_new, Nuclide,prod_uncert_factor,
                                        Muon_scaling,data_from_outlet_only,
                                        this_step_prod_uncert,
                                        this_step_average_production,
                                        is_production_uncertainty_plus_on,
                                        is_production_uncertainty_minus_on);

      // now get the derivative
      N_displace = predict_mean_CRN_conc(eff_e_new+eff_e_displace,Nuclide,
                                       prod_uncert_factor,Muon_scaling, 
                                       data_from_outlet_only,displace_uncertainty,
                                       displace_average_production,
                                       is_production_uncertainty_plus_on,
                                       is_production_uncertainty_minus_on);
    }
    else   // if self and snow sheilding are caluclated based on effective depths
    {
      //cout << "LSDBasin line 1649 You are doing this wih the effective depth driven shielding" << endl;
      
      N_this_step = predict_mean_CRN_conc_with_snow_and_self(eff_e_new, Nuclide,
                                        prod_uncert_factor,
                                        Muon_scaling,data_from_outlet_only,
                                        this_step_prod_uncert,
                                        this_step_average_production,
                                        is_production_uncertainty_plus_on,
                                        is_production_uncertainty_minus_on);
      //cout << " Conc: " << N_this_step << endl;

      // now get the derivative
      N_displace = predict_mean_CRN_conc_with_snow_and_self(eff_e_new+eff_e_displace,Nuclide,
                                       prod_uncert_factor,Muon_scaling, 
                                       data_from_outlet_only,displace_uncertainty,
                                       displace_average_production,
                                       is_production_uncertainty_plus_on,
                                       is_production_uncertainty_minus_on);
    }
    
    f_x =  N_this_step-Nuclide_conc;
    f_x_displace =  N_displace-Nuclide_conc;
    
    N_derivative = (f_x_displace-f_x)/eff_e_displace;
      
    if(N_derivative != 0)
    {
      eff_e_new = eff_e_new-f_x/N_derivative;
      
      // check to see if the difference in erosion rates meet a tolerance
      eff_e_change = f_x/N_derivative;
      //cout << "Change is: " << eff_e_change << " and erosion rate is: " << eff_e_new << endl;
    }
    else
    {
      eff_e_change = 0;
    }
  
  } while(fabs(eff_e_change) > tolerance);

  // replace the production uncertainty
  production_uncertainty = this_step_prod_uncert;
  
  // replace the average production
  average_production = this_step_average_production;
  
  return eff_e_new;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// this function returns the concentration of a nuclide as  function of erosion rate
// The erosion rate should be in g/cm^2/yr
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
double LSDCosmoBasin::predict_mean_CRN_conc(double eff_erosion_rate, string Nuclide,
                                            double prod_uncert_factor, string Muon_scaling,
                                            bool data_from_outlet_only, 
                                            double& production_uncertainty, 
                                            double& average_production,
                                            bool is_production_uncertainty_plus_on,
                                            bool is_production_uncertainty_minus_on)
{
  // production uncertainty factor is a multiplier that sets the production 
  // certainty. If it is 1.1, there is 10% production rate uncertainty, or
  // if it is 0.9 there is -10% unvertainty. The reason why it is implemented
  // like this is that this allows gaussian error propigation.
  if (prod_uncert_factor <=0)
  {
    cout << "You have set an unrealistic production uncertainty factor." << endl;
    cout << "Defaulting to 1." << endl;
    prod_uncert_factor = 1;
  }
  
  // these parameters give the average production rate of the entore basin, 
  // along with the magnitude of the production uncertainty
  double cumulative_production_rate = 0;
  double average_production_rate;
  double average_production_uncertainty;
  
  // the average atoms per gram of the nuclide
  double BasinAverage;
  
  // the total shielding. A product of snow, topographic and production scaling
  double total_shielding;
  double total_shielding_no_uncert;
  
  // the total atomic concentration of the nuclude in question
  double Total_N = 0;
  
  int count_samples = 0;
  
  // initiate a particle. We'll just repeatedly call this particle
  // for the sample. 
  int startType = 0; 
  double Xloc = 0;
  double Yloc = 0;
  double  startdLoc = 0.0;
  double  start_effdloc = 0.0;
  double startzLoc = 0.0;
  
  // create a particle at zero depth
  LSDCRNParticle eroded_particle(startType, Xloc, Yloc,
                               startdLoc, start_effdloc, startzLoc);

  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  
  // set a special case if the outlet flag is true
  int end_node;
  if (data_from_outlet_only)
  {
    end_node = 1;
  }
  else
  {
    end_node =  int(BasinNodes.size());
  }
  
  // set the scaling vector
  vector<bool> nuclide_scaling_switches(4,false);
  if (Nuclide == "Be10")
  {
    nuclide_scaling_switches[0] = true;
  }
  else if (Nuclide == "Al26")
  {
    nuclide_scaling_switches[1] = true;
  }
  else
  {
    cout << "LSDBasin line 1583, You didn't choos a valid nuclide. Defaulting"
         << " to 10Be." << endl;
    Nuclide = "Be10";
    nuclide_scaling_switches[0] = true; 
  }
  
  // check the production uncertainty bools
  if(is_production_uncertainty_plus_on)
  {
    if(is_production_uncertainty_minus_on)
    {
      cout << "You can't have both plus and minus production uncertainty on" << endl;
      cout << "Setting minus uncertainty to false" << endl;
      is_production_uncertainty_minus_on = false;
    }
  }
  
  
  // loop through the elevation data
  for (int q = 0; q < end_node; ++q)
  {
    
    //exclude NDV from average
    if(topographic_shielding[q] != NoDataValue)
    {
      count_samples++;
            
      // reset scaling parameters. This is necessary since the F values are
      // reset for local scaling
      if (Muon_scaling == "Schaller" )
      {
        LSDCRNP.set_Schaller_parameters();
      }
      else if (Muon_scaling == "Braucher" )
      {
        LSDCRNP.set_Braucher_parameters();
      }
      else if (Muon_scaling == "Granger" )
      {
        LSDCRNP.set_Granger_parameters();
      }
      else
      {
        cout << "You didn't set the muon scaling." << endl
             << "Options are Schaller, Braucher and Granger." << endl
             << "You chose: " << Muon_scaling << endl
             << "Defaulting to Braucher et al (2009) scaling" << endl;
        LSDCRNP.set_Braucher_parameters();     
      }
      
      // set the scaling to the correct production uncertainty
      vector<double> test_uncert;
      if(is_production_uncertainty_plus_on)
      {
        test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
      }
      else if(is_production_uncertainty_minus_on)
      {
        test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
      }
      
      // the elevation, snow shielding, topographic shielding
      // and production scaling are all independent of the erosion rate
      // and are calculated seperately. 
      if(  production_scaling.size() < 1 )
      {
        cout << "LSDCosmoBasin, trying to precalculate erosion rate." << endl
             << "Scaling vectors have not been set! You are about to get a seg fault" << endl;
      }
      
      // now you need logic to test if you are accounting for self shielding
      if ( self_shielding.size() < 1 )
      {
        total_shielding_no_uncert = production_scaling[q]*topographic_shielding[q]*
                                  snow_shielding[q];
        total_shielding = prod_uncert_factor*total_shielding_no_uncert;
        cumulative_production_rate += total_shielding_no_uncert;
      }
      else 
      {
        total_shielding_no_uncert = production_scaling[q]*topographic_shielding[q]*
                                  snow_shielding[q]*self_shielding[q];
        total_shielding = prod_uncert_factor*total_shielding_no_uncert;
        cumulative_production_rate += total_shielding_no_uncert;
      }

      LSDCRNP.scale_F_values(total_shielding,nuclide_scaling_switches);
      
      // get the nuclide concentration from this node
      if (Nuclide == "Be10")
      {

        eroded_particle.update_10Be_SSfull(eff_erosion_rate,LSDCRNP);
        Total_N+=eroded_particle.getConc_10Be();
      }
      else if (Nuclide == "Al26")
      {
        eroded_particle.update_26Al_SSfull(eff_erosion_rate,LSDCRNP);
        Total_N+=eroded_particle.getConc_26Al();
      }
      else
      {
        cout << "You didn't give a valid nuclide. You chose: " << Nuclide << endl;
        cout << "Choices are 10Be, 26Al.  Note these case sensitive and cannot" << endl;
        cout << "contain spaces or control characters. Defaulting to 10Be." << endl;
        eroded_particle.update_10Be_SSfull(eff_erosion_rate,LSDCRNP);
        Total_N+=eroded_particle.getConc_10Be();
      }
      
      //cout << endl << endl << "LINE 2052, total shield: " << total_shielding 
      //     << " erosion: " << eff_erosion_rate << " erosion in cm/kyr with rho = 2650: "
      //     << eff_erosion_rate*1e6/2650.0 << " and N: " << eroded_particle.getConc_10Be() << endl;
      
    }                    
  }

  BasinAverage = Total_N/double(count_samples);
  average_production_rate = cumulative_production_rate/double(count_samples);
  average_production_uncertainty = average_production_rate*fabs(1-prod_uncert_factor);
  
  // replace the production uncertanty
  production_uncertainty = average_production_uncertainty;
  
  // replace the average production rate
  average_production = average_production_rate;
      
  return BasinAverage;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// this function returns the concentration of a nuclide as  function of erosion rate
// The erosion rate should be in g/cm^2/yr
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
double LSDCosmoBasin::predict_mean_CRN_conc_with_snow_and_self(double eff_erosion_rate, 
                                            string Nuclide,
                                            double prod_uncert_factor, string Muon_scaling,
                                            bool data_from_outlet_only, 
                                            double& production_uncertainty, 
                                            double& average_production,
                                            bool is_production_uncertainty_plus_on,
                                            bool is_production_uncertainty_minus_on)
{
  // production uncertainty factor is a multiplier that sets the production 
  // certainty. If it is 1.1, there is 10% production rate uncertainty, or
  // if it is 0.9 there is -10% unvertainty. The reason why it is implemented
  // like this is that this allows gaussian error propigation.
  if (prod_uncert_factor <=0)
  {
    cout << "You have set an unrealistic production uncertainty factor." << endl;
    cout << "Defaulting to 1." << endl;
    prod_uncert_factor = 1;
  }
  
  // these parameters give the average production rate of the entore basin, 
  // along with the magnitude of the production uncertainty
  double cumulative_production_rate = 0;
  double average_production_rate;
  double average_production_uncertainty;
  
  // the average atoms per gram of the nuclide
  double BasinAverage;
  
  // the total shielding. A product of snow, topographic and production scaling
  double total_shielding;
  double total_shielding_no_uncert;
  
  // the total atomic concentration of the nuclude in question
  double Total_N = 0;
  
  int count_samples = 0;
  
  // initiate a particle. We'll just repeatedly call this particle
  // for the sample. 
  int startType = 0; 
  double Xloc = 0;
  double Yloc = 0;
  double  startdLoc = 0.0;
  double  start_effdloc = 0.0;
  double startzLoc = 0.0;
  
  // create a particle at zero depth
  LSDCRNParticle eroded_particle(startType, Xloc, Yloc,
                               startdLoc, start_effdloc, startzLoc);

  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  
  // set a special case if the outlet flag is true
  int end_node;
  if (data_from_outlet_only)
  {
    end_node = 1;
  }
  else
  {
    end_node =  int(BasinNodes.size());
  }
  
  // set the scaling vector
  vector<bool> nuclide_scaling_switches(4,false);
  if (Nuclide == "Be10")
  {
    nuclide_scaling_switches[0] = true;
  }
  else if (Nuclide == "Al26")
  {
    nuclide_scaling_switches[1] = true;
  }
  else
  {
    cout << "LSDBasin line 1583, You didn't choose a valid nuclide. Defaulting"
         << " to 10Be." << endl;
    Nuclide = "Be10";
    nuclide_scaling_switches[0] = true; 
  }
  
  // check the production uncertainty bools
  if(is_production_uncertainty_plus_on)
  {
    if(is_production_uncertainty_minus_on)
    {
      cout << "You can't have both plus and minus production uncertainty on" << endl;
      cout << "Setting minus uncertainty to false" << endl;
      is_production_uncertainty_minus_on = false;
    }
  }
  
  // parameters fro the shielding
  double this_top_eff_depth;
  double this_bottom_eff_depth;
  
  // loop through the elevation data
  for (int q = 0; q < end_node; ++q)
  {
    
    //exclude NDV from average
    if(topographic_shielding[q] != NoDataValue)
    {
      count_samples++;
            
      // reset scaling parameters. This is necessary since the F values are
      // reset for local scaling
      if (Muon_scaling == "Schaller" )
      {
        LSDCRNP.set_Schaller_parameters();
      }
      else if (Muon_scaling == "Braucher" )
      {
        LSDCRNP.set_Braucher_parameters();
      }
      else if (Muon_scaling == "Granger" )
      {
        LSDCRNP.set_Granger_parameters();
      }
      else
      {
        cout << "You didn't set the muon scaling." << endl
             << "Options are Schaller, Braucher and Granger." << endl
             << "You chose: " << Muon_scaling << endl
             << "Defaulting to Braucher et al (2009) scaling" << endl;
        LSDCRNP.set_Braucher_parameters();     
      }
      
      // set the scaling to the correct production uncertainty
      vector<double> test_uncert;
      if(is_production_uncertainty_plus_on)
      {
        test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
      }
      else if(is_production_uncertainty_minus_on)
      {
        test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
      }
      
      // the elevation, snow shielding, topographic shielding
      // and production scaling are all independent of the erosion rate
      // and are calculated seperately. 
      if(  production_scaling.size() < 1 )
      {
        cout << "LSDCosmoBasin, trying to precalculate erosion rate." << endl
             << "Scaling vectors have not been set! You are about to get a seg fault" << endl;
      }
      
      // now you need logic to test if you are accounting for self shielding
      total_shielding_no_uncert = production_scaling[q]*topographic_shielding[q];
      total_shielding = prod_uncert_factor*total_shielding_no_uncert;
      cumulative_production_rate += total_shielding_no_uncert;
      
      // scale the F values
      LSDCRNP.scale_F_values(total_shielding,nuclide_scaling_switches);
      
      // check to see if the shielding data exist and if so get the top and bottom
      // effective depths

      
      // first get snow shielding (as implemented by and effective depth of snow)
      if (snow_shield_eff_depth.size() < 1)
      {
        this_top_eff_depth = 0;
      }
      else if (snow_shield_eff_depth.size() == 1)
      {
        this_top_eff_depth = snow_shield_eff_depth[0];
        //cout << "\n\nSnow shield depth: " <<   this_top_eff_depth << endl;
      }
      else
      {
        this_top_eff_depth = snow_shield_eff_depth[q];
        //cout << "\n\nSnow shield depth: " <<   this_top_eff_depth << endl;
      }
      
      // now get the self shielding. This is the thickness of the removed
      // layer
      if (self_shield_eff_depth.size() < 1)
      {
        this_bottom_eff_depth = this_top_eff_depth;
      }
      else if (self_shield_eff_depth.size() == 1)
      {
        this_bottom_eff_depth = this_top_eff_depth+self_shield_eff_depth[0];
        //cout << "\n\n Self shield depth: " << self_shield_eff_depth[0]  << endl;
      }
      else
      {
        this_bottom_eff_depth = this_top_eff_depth+self_shield_eff_depth[q];
        //cout << "\n\n Self shield depth: " << self_shield_eff_depth[q]  << endl;
      }
      
      // get the nuclide concentration from this node
      if (Nuclide == "Be10")
      {
        //cout << "LInE 2271, 10Be" << endl;
        eroded_particle.update_10Be_SSfull_depth_integrated(eff_erosion_rate,LSDCRNP,
                                           this_top_eff_depth, this_bottom_eff_depth);
        Total_N+=eroded_particle.getConc_10Be();
      }
      else if (Nuclide == "Al26")
      {
        //cout << "LINE 2278, 26Al" << endl;
        eroded_particle.update_26Al_SSfull_depth_integrated(eff_erosion_rate,LSDCRNP,
                                           this_top_eff_depth, this_bottom_eff_depth);
        Total_N+=eroded_particle.getConc_26Al();
      }
      else
      {
        cout << "You didn't give a valid nuclide. You chose: " << Nuclide << endl;
        cout << "Choices are 10Be, 26Al.  Note these case sensitive and cannot" << endl;
        cout << "contain spaces or control characters. Defaulting to 10Be." << endl;
        eroded_particle.update_10Be_SSfull_depth_integrated(eff_erosion_rate,LSDCRNP,
                                           this_top_eff_depth, this_bottom_eff_depth);
        Total_N+=eroded_particle.getConc_10Be();         
      }
      
      //cout << endl << endl << "LINE 2052, total shield: " << total_shielding 
      //     << " erosion: " << eff_erosion_rate << " erosion in cm/kyr with rho = 2650: "
      //     << eff_erosion_rate*1e6/2650.0 << " and N: " << eroded_particle.getConc_10Be() << endl;      
      
    }                    
  }

  BasinAverage = Total_N/double(count_samples);
  average_production_rate = cumulative_production_rate/double(count_samples);
  average_production_uncertainty = average_production_rate*fabs(1-prod_uncert_factor);
  
  // replace the production uncertanty
  production_uncertainty = average_production_uncertainty;
  
  // replace the average production rate
  average_production = average_production_rate;
      
  return BasinAverage;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-




//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// this function returns the concentration of a nuclide as  function of erosion rate
// It calculates this based on the elevation scaling of the centroid
// The erosion rate should be in g/cm^2/yr
// NOTE: This is extremely inefficient: it is only here as a test to compare
//  how bad it is vs the full production scaling, but if we start using this more
// the averaging needs to be seperated from the concentration calculations
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
double LSDCosmoBasin::predict_mean_CRN_conc_centroid(double eff_erosion_rate, string Nuclide,
                                            double prod_uncert_factor, string Muon_scaling,
                                            LSDFlowInfo& FlowInfo, 
                                            double& production_uncertainty, 
                                            double& average_production,
                                            bool is_production_uncertainty_plus_on,
                                            bool is_production_uncertainty_minus_on)
{
  // production uncertainty factor is a multiplier that sets the production 
  // certainty. If it is 1.1, there is 10% production rate uncertainty, or
  // if it is 0.9 there is -10% unvertainty. The reason why it is implemented
  // like this is that this allows gaussian error propigation.
  if (prod_uncert_factor <=0)
  {
    cout << "You have set an unrealistic production uncertainty factor." << endl;
    cout << "Defaulting to 1." << endl;
    prod_uncert_factor = 1;
  }
  
  // the average atoms per gram of the nuclide
  double AverageTopo;
  double AverageSnow;
  double AverageProd;
  double AverageSelf;
  
  // the total shielding. A product of snow, topographic and production scaling
  double total_shielding;
  
  // the total atomic concentration of the nuclude in question
  double Total_N = 0;
  
  int count_samples = 0;
  
  // initiate a particle. We'll just repeatedly call this particle
  // for the sample. 
  int startType = 0; 
  double Xloc = 0;
  double Yloc = 0;
  double  startdLoc = 0.0;
  double  start_effdloc = 0.0;
  double startzLoc = 0.0;
  
  // create a particle at zero depth
  LSDCRNParticle eroded_particle(startType, Xloc, Yloc,
                               startdLoc, start_effdloc, startzLoc);

  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;

  // loop through the elevation data, averaging the snow and topo shielding
  double snow_shield_total = 0;
  double topo_shield_total = 0;
  double total_prod_scaling = 0;
  double self_shield_total = 0;
  int centroid_node = 0;   // if the centroid is not in the basin, the 'centroid'
                           // node defaults to the outlet
  int row,col;      // the row and column of the current node
  int end_node = int(BasinNodes.size());
  for (int q = 0; q < end_node; ++q)
  {
    
    //exclude NDV from average
    if(topographic_shielding[q] != NoDataValue)
    {
      count_samples++;
      
      // check to see if this is the centroid
      FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
      
      if(row == Centroid_i && col == Centroid_j)
      {
        centroid_node = q;
      }
      
      if(  production_scaling.size() < 1 )
      {
        cout << "LSDCosmoBasin, trying to precalculate erosion rate." << endl
             << "Scaling vectors have not been set! You are about to get a seg fault" << endl;
      }
      snow_shield_total+= snow_shielding[q];
      topo_shield_total+= topographic_shielding[q];
      total_prod_scaling+= production_scaling[q];
      
      // check for self shielding
      if (self_shielding.size() > 1)
      {
        self_shield_total+= self_shielding[q];
      }
    }
  }

  AverageSnow = snow_shield_total/double(count_samples);
  AverageTopo = topo_shield_total/double(count_samples);
  AverageProd = total_prod_scaling/double(count_samples);
  
  if (self_shielding.size() > 1)
  {
    AverageSelf = self_shield_total/double(count_samples);
  }
  else
  {
    AverageSelf = 1.0;
  }
  
  // at this stage we will try to replicate the basin averaging that goes on in 
  // most paper
  // set scaling parameters. This is necessary since the F values are
  // reset for local scaling
  if (Muon_scaling == "Schaller" )
  {
    LSDCRNP.set_Schaller_parameters();
  }
    else if (Muon_scaling == "Braucher" )
  {
    LSDCRNP.set_Braucher_parameters();
  }
  else if (Muon_scaling == "Granger" )
  {
    LSDCRNP.set_Granger_parameters();
  }
  else
  {
    cout << "You didn't set the muon scaling." << endl
         << "Options are Schaller, Braucher and Granger." << endl
         << "You chose: " << Muon_scaling << endl
         << "Defaulting to Braucher et al (2009) scaling" << endl;
    LSDCRNP.set_Braucher_parameters();     
  }

  // check the production uncertainty bools
  if(is_production_uncertainty_plus_on)
  {
    if(is_production_uncertainty_minus_on)
    {
      cout << "You can't have both plus and minus production uncertainty on" << endl;
      cout << "Setting minus uncertainty to false" << endl;
      is_production_uncertainty_minus_on = false;
    }
  }

  // set the scaling to the correct production uncertainty
  vector<double> test_uncert;
  if(is_production_uncertainty_plus_on)
  {
    test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_plus();
  }
  else if(is_production_uncertainty_minus_on)
  {
    test_uncert = LSDCRNP.set_P0_CRONUS_uncertainty_minus();
  }

  // set the scaling vector
  vector<bool> nuclide_scaling_switches(4,false);
  if (Nuclide == "Be10")
  {
    nuclide_scaling_switches[0] = true;
  }
  else if (Nuclide == "Al26")
  {
    nuclide_scaling_switches[1] = true;
  }
  else
  {
    cout << "LSDBasin line 1583, You didn't choos a valid nuclide. Defaulting"
         << " to 10Be." << endl;
    Nuclide = "Be10";
    nuclide_scaling_switches[0] = true; 
  }


  // now get the shielding. This is based on the average snow sheilding, 
  // the average topo shielding, and the production scaling of the centroid
  double total_shielding_no_uncert = AverageSnow*AverageTopo*AverageSelf*
                                     production_scaling[centroid_node];
  total_shielding = prod_uncert_factor*total_shielding_no_uncert;
                    
  // get the uncertanty
  double average_production_uncertainty = total_shielding_no_uncert*(fabs(1-prod_uncert_factor)); 
                        
                                          
  // now recalculate F values to match the total shielding
  LSDCRNP.scale_F_values(total_shielding,nuclide_scaling_switches);
      
  // get the nuclide concentration from this node
  if (Nuclide == "Be10")
  {
    
    eroded_particle.update_10Be_SSfull(eff_erosion_rate,LSDCRNP);
    Total_N+=eroded_particle.getConc_10Be();
  }
  else if (Nuclide == "Al26")
  {
    eroded_particle.update_26Al_SSfull(eff_erosion_rate,LSDCRNP);
    Total_N+=eroded_particle.getConc_26Al();
  }
  else
  {
    cout << "You didn't give a valid nuclide. You chose: " << Nuclide << endl;
    cout << "Choices are 10Be, 26Al.  Note these case sensitive and cannot" << endl;
    cout << "contain spaces or control characters. Defaulting to 10Be." << endl;
    eroded_particle.update_10Be_SSfull(eff_erosion_rate,LSDCRNP);
    Total_N+=eroded_particle.getConc_10Be();
  }

  // replace the production uncertanty number
  production_uncertainty = average_production_uncertainty;
  
  // replace the average production
  average_production = total_shielding;
  
  
  return Total_N;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// This function gets information of effective elevations for use in
// online calculators 
//
// It returns a vector of values:
//  vector<double> parameter_returns;
//  parameter_returns.push_back(AverageProd);
//  parameter_returns.push_back(AverageTopo);
//  parameter_returns.push_back(AverageSelf);
//  parameter_returns.push_back(AverageSnow);
//  parameter_returns.push_back(AverageCombined);
//  parameter_returns.push_back(lat_outlet);
//  parameter_returns.push_back(outlet_pressure);
//  parameter_returns.push_back(outlet_eff_pressure);
//  parameter_returns.push_back(lat_centroid);
//  parameter_returns.push_back(centroid_pressure);
//  parameter_returns.push_back(centroid_eff_pressure);
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
vector<double> LSDCosmoBasin::calculate_effective_pressures_for_calculators(LSDRaster& Elevation,
                        LSDFlowInfo& FlowInfo, string path_to_atmospheric_data)
{

  // the average atoms per gram of the nuclide
  double AverageTopo;
  double AverageProd;
  double AverageCombined;
  double AverageSnow;
  double AverageSelf;

  // the number of basin pixels
  int count_samples = 0;
  
  // initiate a particle. We'll just repeatedly call this particle
  // for the sample. 
  int startType = 0; 
  double Xloc = 0;
  double Yloc = 0;
  double  startdLoc = 0.0;
  double  start_effdloc = 0.0;
  double startzLoc = 0.0;
  
  // create a particle at zero depth
  LSDCRNParticle eroded_particle(startType, Xloc, Yloc,
                               startdLoc, start_effdloc, startzLoc);

  // now create the CRN parameters object
  LSDCRNParameters LSDCRNP;
  double gamma_spallation = 160;      // in g/cm^2: spallation attentuation depth

  // loop through the elevation data, averaging the snow and topo shielding
  double topo_shield_total = 0;
  double total_prod_scaling = 0;
  double self_shield_total = 0;
  double snow_shield_total = 0;
  double total_combined_scaling = 0;
  double this_snow_shield, this_self_shield;
  int row,col;      // the row and column of the current node
  int end_node = int(BasinNodes.size());
  for (int q = 0; q < end_node; ++q)
  {
    
    //exclude NDV from average
    if(topographic_shielding[q] != NoDataValue)
    {
      count_samples++;
      
      // check to see if this is the centroid
      FlowInfo.retrieve_current_row_and_col(BasinNodes[q], row, col);
      
      if(  production_scaling.size() < 1 )
      {
        cout << "LSDCosmoBasin, trying to precalculate erosion rate." << endl
             << "Scaling vectors have not been set! You are about to get a seg fault" << endl;
      }

      // now get the snow shelding information
      if (snow_shield_eff_depth.size() < 1)
      {
        this_snow_shield = 1;
      }
      else if (snow_shield_eff_depth.size() == 1)
      {
        if (self_shield_eff_depth[0] != 0)
        {
          this_snow_shield = exp(-snow_shield_eff_depth[0]/gamma_spallation);
        }
        else
        {
          this_snow_shield=1;
        }
      }
      else
      {
        if (self_shield_eff_depth[q] != 0)
        {
          this_snow_shield = exp(-snow_shield_eff_depth[q]/gamma_spallation);
        }
        else
        {
          this_snow_shield=1;
        }
      }
      
      // now get the self shelding information
      if (self_shield_eff_depth.size() < 1)
      {
        this_self_shield = 1;
      }
      else if (self_shield_eff_depth.size() == 1)
      {
        if (self_shield_eff_depth[0] != 0)
        {
          this_self_shield = gamma_spallation/self_shield_eff_depth[0]*
                             (1-exp(-self_shield_eff_depth[0]/gamma_spallation));
        }
        else
        {
          this_self_shield = 1;
        }
      }
      else
      {
        if (self_shield_eff_depth[q] != 0)
        {
          this_self_shield = gamma_spallation/self_shield_eff_depth[q]*
                             (1-exp(-self_shield_eff_depth[q]/gamma_spallation));
        }
        else
        {
          this_self_shield=1;
        }
      }
      
      snow_shield_total += this_snow_shield;
      self_shield_total += this_self_shield;
      topo_shield_total += topographic_shielding[q];
      total_prod_scaling += production_scaling[q];
      total_combined_scaling += topographic_shielding[q]*production_scaling[q]*
                                this_snow_shield*this_self_shield;
      
      
    }
  }

  AverageTopo = topo_shield_total/double(count_samples);
  AverageProd = total_prod_scaling/double(count_samples);
  AverageSelf = self_shield_total/double(count_samples);
  AverageSnow = snow_shield_total/double(count_samples);
  AverageCombined = total_combined_scaling/double(count_samples);
  
  // now find the latitude for both the outlet and the centroid
  // first the outlet
  double lat,longitude;
  double lat_centroid, long_centroid;
  double lat_outlet, long_outlet;
  double this_elevation;
  double centroid_pressure, outlet_pressure;
  double centroid_eff_pressure, outlet_eff_pressure;
  
  
  // declare converter object
  LSDCoordinateConverterLLandUTM Converter;

  // get the atmospheric parameters
  LSDCRNP.load_parameters_for_atmospheric_scaling(path_to_atmospheric_data);
  LSDCRNP.set_CRONUS_data_maps();

  Elevation.get_lat_and_long_locations(Outlet_i, Outlet_j, lat, longitude, Converter);
  lat_outlet = lat;
  long_outlet = longitude;
  Elevation.get_lat_and_long_locations(Centroid_i, Centroid_j, lat, longitude, Converter);
  lat_centroid = lat;
  long_centroid = longitude; 
  
  // get outlet and centroid pressures
  this_elevation = Elevation.get_data_element(Centroid_i, Centroid_j);
  centroid_pressure = LSDCRNP.NCEPatm_2(double(lat_centroid), double(long_centroid), 
                                        double(this_elevation));

  this_elevation = Elevation.get_data_element(Outlet_i, Outlet_j);
  outlet_pressure = LSDCRNP.NCEPatm_2(double(lat_outlet), double(long_outlet), 
                                        double(this_elevation));

  // now we use newton iteration to calculate the 'effective' pressure for'
  // both the cnetroid and outlet latitutde.
  // First some variables that are used in the newton iteration
  double f_x,f_x_displace;
  double S_displace, S_this_step;
  double P_displace = 0.01;
  double P_change;
  double P_derivative;
  double tolerance = 1e-6;
  double Fsp = 0.978;
  
  // First for the centroid
  // initial guess is 1000hPa
  double this_P = 1000;
  lat = lat_centroid;
  do
  {
    S_this_step = LSDCRNP.stone2000sp(lat,this_P, Fsp);
    S_displace = LSDCRNP.stone2000sp(lat,this_P+P_displace, Fsp); 
    
    f_x =  S_this_step - AverageProd;
    f_x_displace =  S_displace - AverageProd;
    
    P_derivative =  (f_x_displace-f_x)/P_displace;
    
    if(P_derivative != 0)
    {
      //cout << "Pressure before is: " <<this_P << " lat: " << lat;
      
      this_P = this_P-f_x/P_derivative;
      
      // check to see if the difference in erosion rates meet a tolerance
      P_change = f_x/P_derivative;
      //cout << " Change is: " << P_change << " target is: " << AverageProd << " and Shielding is: " << S_this_step << endl;
      
    }
    else
    {
      P_change = 0;
    }
  } while(fabs(P_change) > tolerance);
  centroid_eff_pressure = this_P;
  
  // Do it again for the outlet
  // initial guess is 1000hPa
  this_P = 1000;
  lat = lat_outlet;
  do
  {
    S_this_step = LSDCRNP.stone2000sp(lat,this_P, Fsp);
    S_displace = LSDCRNP.stone2000sp(lat,this_P+P_displace, Fsp); 
    
    f_x =  S_this_step - AverageProd;
    f_x_displace =  S_displace - AverageProd;
    
    P_derivative =  (f_x_displace-f_x)/P_displace;
    
    if(P_derivative != 0)
    {
      this_P = this_P-f_x/P_derivative;
      
      // check to see if the difference in erosion rates meet a tolerance
      P_change = f_x/P_derivative;
      //cout << "Change is: " << eff_e_change << " and erosion rate is: " << eff_e_new << endl;
    }
    else
    {
      P_change = 0;
    }
  } while(fabs(P_change) > tolerance);
  outlet_eff_pressure = this_P; 
  
  vector<double> parameter_returns;
  parameter_returns.push_back(AverageProd);
  parameter_returns.push_back(AverageTopo);
  parameter_returns.push_back(AverageSelf);
  parameter_returns.push_back(AverageSnow);
  parameter_returns.push_back(AverageCombined);
  parameter_returns.push_back(lat_outlet);
  parameter_returns.push_back(outlet_pressure);
  parameter_returns.push_back(outlet_eff_pressure);
  parameter_returns.push_back(lat_centroid);
  parameter_returns.push_back(centroid_pressure);
  parameter_returns.push_back(centroid_eff_pressure);
    
  return parameter_returns;
  

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This function prints the information, node by node, in the basin.
// Information is printed as a csv file
// It is used for bug checking
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDCosmoBasin::print_particle_csv(string path_to_file, string filename, 
                                       LSDFlowInfo& FlowInfo, 
                                       LSDRaster& Elevation_Data,
                                       LSDRaster& T_Shield,
                                       string path_to_atmospheric_data)
{
  string full_name = path_to_file+filename+".csv";
  ofstream cosmo_out;
  cosmo_out.open(full_name.c_str());
  cosmo_out.precision(8);
  
  cosmo_out << "fID,Easting,Northing,Latitude,Longitude,Elevation,Pressure,TopoShield,Production_scaling,Snowshield" << endl;
  
  // check to see if scaling vecotrs have been made
  int n_nodes = int(BasinNodes.size());
  
  if (n_nodes != int(production_scaling.size()))
  {
    cout << "LSDCosmoBasin Line 1119, printing node info to csv but am getting shielding first." << endl;
    populate_scaling_vectors(FlowInfo, Elevation_Data, T_Shield, path_to_atmospheric_data);
  }

  // the latitude and longitude
  double lat,longitude;
  float Easting, Northing;
  double this_pressure,this_elevation,this_SShield,this_TShield,this_PShield;
  int row,col;
  
  // decalre converter object
  LSDCoordinateConverterLLandUTM Converter;
  
  // get the CRN parameters
  LSDCRNParameters LSDCRNP;
  
  // get the atmospheric parameters
  LSDCRNP.load_parameters_for_atmospheric_scaling(path_to_atmospheric_data);
  LSDCRNP.set_CRONUS_data_maps();
  
  // now loop through nodes, printing the location and scaling
  for(int n = 0; n < n_nodes; n++)
  {
    FlowInfo.retrieve_current_row_and_col(BasinNodes[n], row, col);
    
    //exclude NDV from average
    if (Elevation_Data.get_data_element(row,col) != NoDataValue)
    {
      // To get pressure, first get the lat and long
      Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude, Converter);
      Elevation_Data.get_x_and_y_locations(row, col, Easting, Northing);
      //Elevation_Data.get_lat_and_long_locations(row, col, lat, longitude);


      // now the pressure
      this_elevation = Elevation_Data.get_data_element(row,col);
      this_pressure = LSDCRNP.NCEPatm_2(double(lat), double(longitude), 
                                        double(this_elevation));

      // get the shielding
      this_TShield = topographic_shielding[n];
      this_SShield = snow_shielding[n];
      this_PShield = production_scaling[n];      

      // now print to file
      cosmo_out << n+1 << ","<<Easting<<","<<Northing<<","<<lat<<","<<longitude
                <<","<<this_elevation<<","<<this_pressure<<","<<this_TShield<<","
                <<this_PShield<<","<<this_SShield<< endl;
    }
  }
}




#endif