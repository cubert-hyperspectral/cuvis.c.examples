#include "cuvis.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
  /*
  * Load and change a measurement
  *
  * In this example a measurement is loaded and the distance is changed.
  *
  * Used principles:
  *   - *SessionFile* to load a recorded measurement
  *
  * Run properties
  *   - "path/to/settings" path/to/sesstion/file.cu3s" distanceInMm "OutputFolderName"
  */
  if (argc != 5)
  {
    printf("To few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("sessionfile (.cu3s)\n");
    printf("new distance\n");
    printf("Name of export directory\n");
    fflush(stdout);

    return -1;
  }

  char* const userSettingsDir = argv[1];
  char* const sessionLoc = argv[2];
  char* const distanceString = argv[3];
  char* const exportDir = argv[4];

  int distance = atoi(distanceString); // in mm

  printf("Example 08 change distance \n");
  printf(userSettingsDir);
  printf("\nsessionfile (.cu3s): ");
  printf(sessionLoc);
  printf("\nNew Distance: %d\n", distance);
  printf("Export Dir: ");
  printf(exportDir);
  fflush(stdout);

  CUVIS_SESSION_FILE sess;

  CUVIS_MESU mesu;
  CUVIS_MESU_METADATA mesu_data = {0};

  CUVIS_PROC_CONT procCont;

  CUVIS_INT is_capable;

  /*
  * Settings
  * Initialize the Cuvis SDK using a settings - directory
  * This is optional(all settings have defaults),
  * but enables you to optimize Cuvis' performance on your system using the settings
  * Your camera and the default Cuvis installation both provide these settings files
  */
  printf("\nloading settings... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_debug, NULL));
#ifdef _DEBUG
  CUVIS_CHECK(cuvis_set_log_level(loglevel_debug));
#else
  CUVIS_CHECK(cuvis_set_log_level(loglevel_info));
#endif

  printf("\nloading session... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_load(sessionLoc, &sess));

  printf("\nloading measurement... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_get_mesu(sess, 0, session_item_type_frames_no_gaps, &mesu));

  CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
  printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
  fflush(stdout);

  /*
  * Processing Context
  * The *ProcessingContext* is the interface that enables computing a hyperspectral cube from a measurement.
  * A camera calibration file is required to initialize the *ProcessingContext*, as each Cubert camera is individually calibrated to provide the most accurate spectral information.
  * As a SessionFile contains the camera calibration, it is used to construct the *ProcessingContext* .
  *
  * To generate a hyperspectral cube, the *ProcessingContext* is **applied** to the *Measurement* .The *Measurement* is modified **in-place** and now contains a cube.
  *
  * To select the processing mode, write the `processing_mode` attribute.
  *
  * When initializing a *ProcessingContext* from a *SessionFile*, the reference *Measurements* stored in the *SessionFile* are automatically loaded and set within the *ProcessingContext* .
  * Using the method `set_reference`, different measurements can be set for each reference type.
  */
  printf("Load calibration and processing context...");
  fflush(stdout);
  CUVIS_INT load_references = 1;
  CUVIS_CHECK(cuvis_proc_cont_create_from_session_file(sess, load_references, &procCont));
  printf(" done. \n");
  fflush(stdout);

  printf("prepare saving of measurements... \n");
  fflush(stdout);
  CUVIS_EXPORTER cube_exporter;

  // pan sharpening settings
  CUVIS_PANSHARPENING_SETTINGS ps_settings = {
      "all",                                             // CUVIS_CHAR channel_selection[CUVIS_MAXBUF]
      1,                                                 // float spectra_multiplier
      0.0,                                               // double pan_scale
      pan_sharpening_interpolation_type_NearestNeighbor, // CUVIS_PAN_SHAPRENING_INTERPOLATION_TYPE pan_interpolation_type
      pan_sharpening_algorithm_Noop,                     // CUVIS_PAN_SHAPRENING_ALGORITHM_TYPE pan_algorithm
      0,                                                 // CUVIS_INT pre_pan_sharpen_cube
      0,                                                 // CUVIS_INT add_pan
  };

  // Export settings: General processing of measurements
  CUVIS_EXPORT_GENERAL_SETTINGS general_settings = {
      "",          // CUVIS_CHAR export_dir[CUVIS_MAXBUF]: set below (initializer only takes const char*)
      0,           // CUVIS_INT add_fullscale_pan
      0,           // CUVIS_INT permissive: skip errors where possible
      ps_settings, // CUVIS_PANSHARPENING_SETTINGS pansharpening_settings
  };

  strcpy(general_settings.export_dir, exportDir);

  CUVIS_EXPORT_CUBE_SETTINGS cube_settings = {
      session_merge_mode_Default, // CUVIS_SESSION_MERGE_MODE merge_mode
      1,                          // CUVIS_INT allow_overwrite
      0,                          // CUVIS_INT allow_drop: drop frames when the writer cannot keep up
      1,                          // CUVIS_INT allow_session_file: write a .cu3s file
      1,                          // CUVIS_INT allow_info_file: write a frame list next to it
      OperationMode_Software,     // CUVIS_OPERATION_MODE operation_mode
      0.0,                        // double fps: only for OperationMode_Internal
      20,                         // CUVIS_INT soft_limit: buffer size for reordering frames
      100,                        // CUVIS_INT hard_limit: max buffered frames
      10000,                      // CUVIS_INT max_buftime: max time a frame is buffered [ms]
      0,                          // CUVIS_INT full_export: include the cube
  };

  cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings);

  /*
  * Distance Calibration
  * Most Ultris cameras (except for relay variants) require distance calibration to achieve optimal results.
  *
  * Please note: Setting the distance in mm is not supported for R models (relay optics, e.g. Ultris XMR) and P models (pan sensor, e.g. Ultris X20P). P models support distance calibration with a distance reference measurement instead (see Example 3).
  *
  * Distance calibration is an operation that can be done with already recorded data and requires a distance reference measurement.
  * The reference should contain high-contrast data over the relevant spectral channels at the desired distance that data should be calibrated to.
  *
  * If the distance to the object is known, it can also be set directly in mm, without a distance reference measurement. This example does that.
  */
  printf("Set distance ...");
  fflush(stdout);
  // no CUVIS_CHECK: fails on R and P models, the example continues without it
  cuvis_proc_cont_calc_distance(procCont, distance);
  printf(" done. \n");
  fflush(stdout);

  CUVIS_PROC_ARGS args = {
      Cube_Raw, // CUVIS_PROCESSING_MODE processing_mode
      0,        // CUVIS_INT allow_recalib
  };

  CUVIS_CHECK(cuvis_proc_cont_is_capable(procCont, mesu, args, &is_capable));

  if (1 == is_capable)
  {
    printf("reprocess measurement to Cube_Raw with custom distance mode...");
    fflush(stdout);

    CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));
    CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
    printf(" done. \n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
    printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
    fflush(stdout);

    cuvis_exporter_apply(cube_exporter, mesu);
  }
  else
  {
    printf("Cannot process to Cube_Raw mode.\n");
    fflush(stdout);
  }

  cuvis_exporter_free(&cube_exporter);
  cuvis_proc_cont_free(&procCont);
  cuvis_measurement_free(&mesu);
  cuvis_shutdown();
  printf("finished.");
  fflush(stdout);
}
