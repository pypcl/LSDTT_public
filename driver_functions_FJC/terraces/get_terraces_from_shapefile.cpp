//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// terraces_swath_driver.cpp
// Extract information about terraces using a shapefile of the main stem channel.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Fiona Clubb
// University of Edinburgh
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include "../LSDRaster.hpp"
#include "../LSDSwathProfile.hpp"
#include "../LSDShapeTools.hpp"
#include "../LSDJunctionNetwork.hpp"
#include "../LSDTerrace.hpp"
#include "../LSDParameterParser.hpp"
#include "../LSDSpatialCSVReader.hpp"
#include "../LSDGeometry.hpp"
#include "../LSDRasterInfo.hpp"

int main (int nNumberofArgs,char *argv[])
{
	//start the clock
	clock_t begin = clock();

	//Test for correct input arguments
  if (nNumberofArgs!=3)
  {
    cout << "=========================================================" << endl;
    cout << "|| Welcome to the terrace swath tool!  	              ||" << endl;
    cout << "|| This program takes in a shapefile of points and     ||" << endl;
		cout << "|| finds the nearest pixel crossing a channel of a     ||" << endl;
		cout << "|| specified straem order. It then creates a swath     ||" << endl;
		cout << "|| between the outlet and the longest channel in the   ||" << endl;
		cout << "|| basin. It then extracts the terraces along the   		||" << endl;
		cout << "|| swath using slope and relief thresholds.	    		  ||" << endl;
    cout << "|| This program was developed by                       ||" << endl;
    cout << "|| Fiona J. Clubb												              ||" << endl;
    cout << "|| at the University of Edinburgh                     ||" << endl;
    cout << "=========================================================" << endl;
    cout << "This program requires two inputs: " << endl;
    cout << "* First the path to the parameter file." << endl;
    cout << "* Second the name of the param file (see below)." << endl;
    cout << "---------------------------------------------------------" << endl;
    cout << "Then the command line argument will be, for example: " << endl;
    cout << "In linux:" << endl;
    cout << "./get_terraces_from_shapefile.out /LSDTopoTools/Topographic_projects/Test_data/ LSDTT_terraces.param" << endl;
    cout << "=========================================================" << endl;
    exit(EXIT_SUCCESS);
  }
  string path_name = argv[1];
  string f_name = argv[2];

	// maps for setting default parameters
	map<string,int> int_default_map;
	map<string,float> float_default_map;
	map<string,bool> bool_default_map;
	map<string,string> string_default_map;

	// set default int parameters
	int_default_map["Threshold_SO"] = 3;
	int_default_map["Relief lower percentile"] = 25;
	int_default_map["Relief upper percentile"] = 75;
	int_default_map["Slope lower percentile"] = 25;
	int_default_map["Slope upper percentile"] = 75;
	int_default_map["Min patch size"] = 1000;
	int_default_map["search_radius"] = 10;
	int_default_map["NormaliseToBaseline"] = 1;
	int_default_map["SO_outlets"] = 5;

	// set default float parameters
	float_default_map["surface_fitting_window_radius"] = 6;
	float_default_map["Min slope filling"] = 0.0001;
	float_default_map["QQ threshold"] = 0.005;
	float_default_map["HalfWidth"] = 500;
	float_default_map["Min terrace height"] = 5;
	float_default_map["DrainageAreaThreshold"] = 10000;

	// set default bool parameters
	bool_default_map["Filter topography"] = true;

	// set default string parameters
	string_default_map["input_shapefile"] = "NULL";

	// Use the parameter parser to get the maps of the parameters required for the
	// analysis
	// load parameter parser object
	LSDParameterParser LSDPP(path_name,f_name);
	LSDPP.force_bil_extension();

	LSDPP.parse_all_parameters(float_default_map, int_default_map, bool_default_map,string_default_map);
	map<string,float> this_float_map = LSDPP.get_float_parameters();
	map<string,int> this_int_map = LSDPP.get_int_parameters();
	map<string,bool> this_bool_map = LSDPP.get_bool_parameters();
	map<string,string> this_string_map = LSDPP.get_string_parameters();

	// Now print the parameters for bug checking
	LSDPP.print_parameters();

	// location of the files
	string DATA_DIR =  LSDPP.get_read_path();
	string DEM_ID =  LSDPP.get_read_fname();
	string OUT_DIR = LSDPP.get_write_path();
	string OUT_ID = LSDPP.get_write_fname();
	string DEM_extension =  LSDPP.get_dem_read_extension();
	vector<string> boundary_conditions = LSDPP.get_boundary_conditions();
	string CHeads_file = LSDPP.get_CHeads_file();

	// some error checking
	if (CHeads_file.empty())
	{
		cout << "FATAL ERROR: I can't find your channel heads file. Check your spelling!! \n The parameter key needs to be 'channel heads fname'" << endl;
		exit(EXIT_SUCCESS);
	}

  cout << "starting the test run... here we go!" << endl;

	LSDRaster RasterTemplate;
	LSDRasterInfo RasterInfo;

	if(this_bool_map["Filter topography"])
	{
		 // load the DEM
		 cout << "Loading the DEM..." << endl;
		 LSDRaster load_DEM((DATA_DIR+DEM_ID), DEM_extension);
		 RasterTemplate = load_DEM;
		 LSDRasterInfo get_RI((DATA_DIR+DEM_ID), DEM_extension);
		 RasterInfo = get_RI;

		 // filter using Perona Malik
		 int timesteps = 50;
		 float percentile_for_lambda = 90;
		 float dt = 0.1;
		 RasterTemplate = RasterTemplate.PeronaMalikFilter(timesteps, percentile_for_lambda, dt);

		 // fill
		 RasterTemplate = RasterTemplate.fill(this_float_map["Min slope filling"]);
		 string fill_name = "_filtered";
		 RasterTemplate.write_raster((DATA_DIR+DEM_ID+fill_name), DEM_extension);
	}
	else
	{
		//don't do the filtering, just load the filled DEM
		LSDRaster load_DEM((DATA_DIR+DEM_ID+"_Fill"), DEM_extension);
		RasterTemplate = load_DEM;
		LSDRasterInfo get_RI((DATA_DIR+DEM_ID), DEM_extension);
		RasterInfo = get_RI;
	}

	cout << "\t Flow routing..." << endl;
	// get a flow info object
	LSDFlowInfo FlowInfo(boundary_conditions, RasterTemplate);

	cout << "\t Loading the sources" << endl;
	// calcualte the distance from outlet
	LSDRaster DistanceFromOutlet = FlowInfo.distance_from_outlet();
	// load the sources
	vector<int> sources = FlowInfo.Ingest_Channel_Heads((DATA_DIR+CHeads_file), "csv", 2);
	cout << "\t Got sources!" << endl;

	// now get the junction network
	LSDJunctionNetwork ChanNetwork(sources, FlowInfo);
  cout << "\t Got the channel network" << endl;

 	// reading in the shapefile with the points
	PointData ProfilePoints = LoadShapefile(path_name+this_string_map["input_shapefile"].c_str());

	// get the utm information
	vector<double> UTME = ProfilePoints.X;
	vector<double> UTMN = ProfilePoints.Y;
	int UTMZone = 0;
	bool is_North = false;
	RasterTemplate.get_UTM_information(UTMZone,is_North);

	// create the polyline
	LSDPolyline Line(UTME,UTMN,UTMZone);
	// get the pixels along the line
	vector<int> line_rows;
	vector<int> line_cols;
	Line.get_affected_pixels_in_line(RasterInfo, line_rows, line_cols);
	vector<int> line_nodes = Line.get_flowinfo_nodes_of_line(RasterInfo, FlowInfo);
	string line_ext = "_line";
	FlowInfo.print_vector_of_nodeindices_to_csv_file(line_nodes, (DATA_DIR+DEM_ID+line_ext));

	vector<int> outlet_nodes = ChanNetwork.get_channel_pixels_along_line(line_rows, line_cols, this_int_map["SO_outlets"], FlowInfo);

	// print nodes to csv for checking
	string outlet_ext = "_outlet";
	FlowInfo.print_vector_of_nodeindices_to_csv_file(outlet_nodes, (DATA_DIR+DEM_ID+outlet_ext));

	// get the nearest upslope junction from these nodes
	vector<int> outlet_jns = ChanNetwork.extract_basin_junctions_from_nodes(outlet_nodes, FlowInfo);
	int n_basins = outlet_jns.size();

	cout << "The number of outlet nodes is: " << n_basins << endl;

	// get the longest channel for each basin
	vector<int> basin_sources = ChanNetwork.get_basin_sources_from_outlet_vector(outlet_jns, FlowInfo, DistanceFromOutlet);

	//initialise empty raster for merging terraces together
	int NDV = RasterTemplate.get_NoDataValue();
	LSDIndexRaster TerraceLocations(RasterTemplate, NDV);
	LSDRaster TerraceElevations = RasterTemplate.create_raster_nodata();

	for (int i =0; i < n_basins; i++)
	{
		// assign info for this basin
		int upstream_node = basin_sources[i];
		int downstream_node = outlet_nodes[i];
		string jn_name = itoa(outlet_jns[i]);
		string uscore = "_";
		jn_name = uscore+jn_name;

		// get the channel between the outlet and the upstream junction
		LSDIndexChannel BaselineChannel(upstream_node, downstream_node, FlowInfo);
		vector<double> X_coords;
		vector<double> Y_coords;
		BaselineChannel.get_coordinates_of_channel_nodes(X_coords, Y_coords);

		// get the point data from the BaselineChannel
		PointData BaselinePoints = get_point_data_from_coordinates(X_coords, Y_coords);

	  cout << "\t Creating swath template" << endl;
	  LSDSwath TestSwath(BaselinePoints, RasterTemplate, this_float_map["HalfWidth"]);

		cout << "\n\t Getting raster from swath" << endl;
		LSDRaster SwathRaster = TestSwath.get_raster_from_swath_profile(RasterTemplate, this_int_map["NormaliseToBaseline"]);
		string swath_ext = "_swath_raster";
		SwathRaster.write_raster((DATA_DIR+DEM_ID+swath_ext+jn_name), DEM_extension);

	  // get the slope
		cout << "\t Getting the slope" << endl;
	  vector<LSDRaster> surface_fitting;
	  LSDRaster Slope;
	  vector<int> raster_selection(8, 0);
	  raster_selection[1] = 1;             // this means you want the slope
	  surface_fitting = RasterTemplate.calculate_polyfit_surface_metrics(this_float_map["surface_fitting_window_radius"], raster_selection);
	  Slope = surface_fitting[1];

		float mask_threshold = 1.0;
		bool below = 0;
		// remove any stupid slope values
		LSDRaster Slope_new = Slope.mask_to_nodata_using_threshold(mask_threshold, below);

		// get the channel relief and slope threshold using quantile-quantile plots
		cout << "Getting channel relief threshold from QQ plots" << endl;
		string qq_fname = DATA_DIR+DEM_ID+"_qq_relief.txt";
		float relief_threshold_from_qq = SwathRaster.get_threshold_for_floodplain_QQ(qq_fname, this_float_map["QQ threshold"], this_int_map["Relief lower percentile"], this_int_map["Relief upper percentile"]);

		cout << "Getting slope threshold from QQ plots" << endl;
		string qq_slope = DATA_DIR+DEM_ID+"_qq_slope.txt";
		float slope_threshold_from_qq = Slope_new.get_threshold_for_floodplain_QQ(qq_slope, this_float_map["QQ threshold"], this_int_map["Slope lower percentile"], this_int_map["Slope upper percentile"]);

		cout << "Relief threshold: " << relief_threshold_from_qq << " Slope threshold: " << slope_threshold_from_qq << endl;

		cout << "Removing pixels within " << this_float_map["Min terrace height"] << " m of the modern channel" << endl;
		// get the terrace pixels
		LSDTerrace Terraces(SwathRaster, Slope_new, ChanNetwork, FlowInfo, relief_threshold_from_qq, slope_threshold_from_qq, this_int_map["Min patch size"], this_int_map["Threshold_SO"], this_float_map["Min terrace height"]);
		LSDIndexRaster ConnectedComponents = Terraces.print_ConnectedComponents_to_Raster();
		// add to raster
		TerraceLocations.MergeIndexRasters(ConnectedComponents);

		cout << "\t Testing connected components" << endl;
		vector <vector <float> > CC_vector = TestSwath.get_connected_components_along_swath(ConnectedComponents, RasterTemplate, this_int_map["NormaliseToBaseline"]);

		// push back results to file for plotting
		ofstream output_file_CC;
		string output_fname = "_terrace_swath_plots"+jn_name+".txt";
		output_file_CC.open((DATA_DIR+DEM_ID+output_fname).c_str());
		for (int i = 0; i < int(CC_vector[0].size()); ++i)
		{
			output_file_CC << CC_vector[0][i] << " " << CC_vector[1][i] << " " << CC_vector[2][i] << endl;
		}
		output_file_CC.close();

		// write raster of terrace elevations
		LSDRaster ChannelRelief = Terraces.get_Terraces_RasterValues(SwathRaster);
		TerraceElevations.OverwriteRaster(ChannelRelief);
	}

	string CC_ext = "_terrace_IDs";
	TerraceLocations.write_raster((DATA_DIR+DEM_ID+CC_ext), DEM_extension);

	string relief_ext = "_terrace_relief_final";
	TerraceElevations.write_raster((DATA_DIR+DEM_ID+relief_ext), DEM_extension);

	// Done, check how long it took
	clock_t end = clock();
	float elapsed_secs = float(end - begin) / CLOCKS_PER_SEC;
	cout << "DONE, Time taken (secs): " << elapsed_secs << endl;
}