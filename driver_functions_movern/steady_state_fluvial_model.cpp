//----------------------------------------------------------------------------//
// steady_state_fluvial_model.cpp
//
// This script creates a steady state landscape for a specific m/n landscape.
// The model is run in 3 stages:
//
// 1. The initial surface is created. Firstly, we generate a fractal surface
// using a diamond square algorithm with a desired relief.  We then taper the
// edges of the domain to sea level.  After the fractal surface is generated
// we add a parabola to the topography in order to stop lots of random depressions
// in the landscape. The user needs to specify the relief of the parabola - at
// the moment this is set to fractal_relief/2.
//
// 2. The initial surface is fully dissected by setting a high K value and uplift
// rate. We then run the model for 50,000 years.
//
// 3. The appropriate parameters are set using the chi values. We calculate chi
// for every point in the model, and use this to calculate the appropriate K value
// to get a desired relief of the final model domain.  We then use this K value
// and uplift rate to calculate the elevation of the surface, and snap to steady
// state.
//
// Authors: Simon M. Mudd and Fiona J. Clubb
// University of Edinburgh
//----------------------------------------------------------------------------//

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include "../LSDRasterModel.hpp"
#include "../LSDParticleColumn.hpp"
#include "../LSDParticle.hpp"
#include "../LSDCRNParameters.hpp"
using namespace std;

bool file_check(string name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main(int argc, char *argv[])
{
  LSDRasterModel mod;
  stringstream ss;
  string pathname;
  string start_ext;
  string full_start_name;

  if (argc <= 1)
  {
    cout << "ERROR: you need to enter a data folder for the model outputs" << endl;
    exit(0);
  }
  else
  {
    pathname = argv[1];
    string lchar = pathname.substr(pathname.length()-2,1);
    string slash = "/";
    cout << "lchar is " << lchar << " and slash is " << slash << endl;

    if (lchar != slash)
    {
      cout << "You forgot the frontslash at the end of the path. Appending." << endl;
      pathname = pathname+slash;
    }
    cout << "The pathname is: " << pathname << endl;
  }

  // set the domain size
  // first change the default dimensions
  int newrows = 540;
  int newcols = 1080;
  float datares = 30;
  mod.resize_and_reset(newrows,newcols,datares);

  // set the end time, print interval, etc
  mod.set_K(0.00001);
  mod.set_endTime(50000);
  mod.set_print_interval(5);
  mod.set_print_hillshade(true);

  string template_param = "template_param.param";
  string full_template_name = pathname+template_param;
  mod.make_template_param_file(full_template_name);

  // add the path to the default filenames
  mod.add_path_to_names( pathname);

  // add random asperities to the surface of default model
  mod.random_surface_noise();

  // STEP 1: INITIAL CONDITIONS
  cout << "Firstly I'm getting the initial fractal parabolic surface" << endl;

  // This is the desired relief of the fractal surface. If this is large, it takes longer
  // for the channels to fully dissect the landscape, but if it is too low
  // I think you will get very straight channels.
  float desired_relief = 50;

  // run to steady state using a large fluvial incision rate
  // (I don't know why but this seems to encourage complete dissection)
  // and turn the hillslopes off, just to save some time
  mod.set_hillslope(false);

  cout << "I am going to start with a fractal surface" << endl;

  // The feature order sets the largest wavelength of repeating features.
  // The number of pixels of the largest feature is this 2^n where n is the
  // feature order
  int feature_order = 9;
  mod.intialise_diamond_square_fractal_surface(feature_order, desired_relief);

  // Make sure the edges are tapered
  int rows_to_taper = 5;
  mod.initialise_taper_edges_and_raise_raster(rows_to_taper);

  // add a bit of noise to get rid of straight channels on edge
  float noise_amp = 0.1;
  mod.set_noise(noise_amp);
  mod.random_surface_noise();

  // add a parabola to the topography
  // It is set with a relief. If this is similar to the relief of the fractal
  // surface then you are less likely to get straight channels in the middle
  // of the landscape where the model has had to fill low points created by the
  // fractal algorithm, but you will be more likely to get parallel rather
  // than dendritic channels.
  float parabolic_relief = desired_relief/4;
  mod.superimpose_parabolic_surface(parabolic_relief);
  cout << "Finished with the fractal surface " << endl;

  int frame = 555;
  mod.print_rasters(frame);

  // STEP 2: DISSECT THE LANDSCAPE
  // Set a relatively high K value and a high uplift rate to dissect the landscape
  // This happens quite fast at these settings! You might not even need
  // 50000 years and could possible shorten the end time.
  cout << "Dissecting landscape." << endl;
  mod.set_endTime(30000);
  mod.set_timeStep( 250 );
  mod.set_K(0.01);
  float new_baseline_uplift = 0.0025;
  mod.set_baseline_uplift(new_baseline_uplift);
  mod.set_current_frame(1);
  mod.run_components_combined();

  // a bit more dissection
  // I guess I just do this to make sure full dissection occurs.
  // You might reduce the end time. Note that the time is cumulative so this has
  // to be greater than the previous end time.
  mod.set_endTime(60000);
  mod.set_K(0.001);
  float U = 0.0001;    // a tenth of a mm per year
  mod.set_baseline_uplift(U);
  mod.run_components_combined();


  // STEP 3: SET TO STEADY STATE
  // This gives you the correct relief for a given U by back calculating K.
  // It is calculated directly from the chi plots.
  // Note that if the landscape is not completely dissected you will get weird
  // looking landscapes at this point.

  // Note that if you want to change m or n use
  // mod.set_m(0.5);
  // mod.set_n(1.5);
  // the m/n ratio will be calculated from these.
  mod.set_baseline_uplift(U);
  desired_relief = 1000;
  float new_K = mod.fluvial_snap_to_steady_state_tune_K_for_relief(U, desired_relief);
  cout << "Getting a steady solution for a landscape with relief of " << desired_relief
       << " metres and uplift of " << U*1000 << " mm per year." << endl;
  cout << "The new K is: " << new_K << endl;

  cout << "Now let me print the raster for you" << endl;
  mod.set_print_hillshade(true);

  // The frames are added to the filenames, so these set specific filenames
  // for, in this case, the steady state landscape.
  frame = 999;
  mod.print_rasters(frame);

  // add some time
  mod.set_endTime(100000);
  mod.run_components_combined();

  return 0;
  }