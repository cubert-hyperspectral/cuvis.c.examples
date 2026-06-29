#include "cuvis.h"

#include <stdio.h>
#include <stdlib.h>

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

  printf("Example 07 change distance cpp \n");
  printf(userSettingsDir);
  printf("\nsessionfile (.cu3s): ");
  printf(sessionLoc);
  printf("\nNew Distance: %d\n", distance);
  printf("Export Dir: ");
  printf(exportDir);
  fflush(stdout);

  CUVIS_SESSION_FILE sess;

  CUVIS_MESU mesu;
  CUVIS_MESU_METADATA mesu_data;

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
      "all",
      1,
      0.0,
      pan_sharpening_interpolation_type_NearestNeighbor,
      pan_sharpening_algorithm_Noop,
      0,
      0};

  // Export settings: General processing of measurements
  CUVIS_EXPORT_GENERAL_SETTINGS general_settings = {
      "", // initializer list only takes const char*, leave empty and modify afterwards.
      0,
      0,
      ps_settings};

  strcpy(general_settings.export_dir, exportDir);

  CUVIS_EXPORT_CUBE_SETTINGS cube_settings;
  cube_settings.merge_mode = session_merge_mode_Default;
  cube_settings.allow_overwrite = 1;
  cube_settings.allow_session_file = 1;

  cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings);

  /*
  * Distance Calibration
  * Most Ultris cameras (except for Relay-variants) require distance calibration to achieve optimal results.
  *
  * Please note: The provided default demo dataset was recorded with a relay-equipped camera (Ultris XM with relay optics). Thus this step is not applicable to this dataset.
  *
  * Distance calibration is an operation that can be done with already recorded data and requires a distance reference measurement.
  * The reference should contain high-contrast data over the relevant spectral channels at the desired distance that data should be calibrated to.
  *
  * In this example, the measurement itself will be used as the distance reference.If the target object is suitable (high contrast, non-repeating patterns), this can suffice for good results.
  */
  printf("Set distance ...");
  fflush(stdout);
  // CUVIS_CHECK(cuvis_proc_cont_calc_distance(procCont, distance)); // throws error on pan image
  cuvis_proc_cont_calc_distance(procCont, distance);
  printf(" done. \n");
  fflush(stdout);

  CUVIS_PROC_ARGS args;
  args.processing_mode = Cube_Raw;

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
