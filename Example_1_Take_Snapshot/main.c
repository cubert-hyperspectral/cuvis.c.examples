#include "cuvis.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
  #include <Windows.h>
#else
  #include <unistd.h>
#endif

bool is_directory(const char* path)
{
  struct stat s = {0};
  if (stat(path, &s) == 0)
  {
    return (s.st_mode & S_IFDIR) != 0;
  }
  return false;
}

bool is_file(const char* path)
{
  struct stat s = {0};
  if (stat(path, &s) == 0)
  {
    return (s.st_mode & S_IFREG) != 0;
  }
  return false;
}

int main(int argc, char* argv[])
{
  /*
  * Connect to camera and record a measurement
  *
  * This example provides a minimal starting point to allow you to get a camera and data acquisition running.
  *
  * Used principles:
  *   - *AcquisitionContext* for camera control and data acquisition
  *   - *SessionFile* as camera calibration file
  *   - *CubeExporter* for saving measurements
  *
  * Prerequisites to running this example:
  *   - Have a camera connected *or* downloaded the provided [demo data](https://cloud.cubert-gmbh.de/s/SDKSampleData)
  *   - Have the camera calibration file (*SN*.cu3c) ready *or* use the [demo data](https://cloud.cubert-gmbh.de/s/SDKSampleData)
  *   - Have the Cuvis SDK installed
  *
  * Run properties:
  *   - "path/to/settings" path/to/factory" "path/to/recording/directory" exPoseTimeInMs numberOfImages
  */
  if (argc != 6)
  {
    printf("To few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("factory directory\n");
    printf("path of recording directory\n");
    printf("exposure time in ms\n");
    printf("number of images\n");
    return -1;
  }

  char* const userSettingsDir = argv[1];
  char* const factoryDir = argv[2];
  char* const recDir = argv[3];
  char* const exposureString = argv[4]; // in ms
  char* const nrImagesString = argv[5];

  int exposure_ms = atoi(exposureString);
  int nrImages = atoi(nrImagesString);

  printf("Example 01 record single image\n");
  printf("User Settings directory: ");
  printf(userSettingsDir);
  printf("\nFactory directory: ");
  printf(factoryDir);
  printf("\nRecording directory: ");
  printf(recDir);
  printf("\nExposure in ms: %d\n", exposure_ms);
  printf("Number of images: %d\n", nrImages);

  CUVIS_CALIB calib;
  CUVIS_ACQ_CONT acqCont;
  CUVIS_PROC_CONT procCont;

  /*
  * Settings
  * Initialize the Cuvis SDK using a settings - directory
  * This is optional(all settings have defaults),
  * but enables you to optimize Cuvis' performance on your system using the settings
  * Your camera and the default Cuvis installation both provide these settings files
  */
  printf("load user settings...\n");
  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_debug, NULL));
  cuvis_set_log_level(loglevel_info);

  printf("load calibration...\n");

  if (is_directory(factoryDir))
  {
    CUVIS_CHECK(cuvis_calib_create_from_path(factoryDir, &calib));
  }
  else if (is_file(factoryDir) && strstr(factoryDir, ".cu3c") != NULL)
  {
    printf("using .cu3c file as calibration instead of factory dir...\n");

    CUVIS_SESSION_FILE sessionFile;
    CUVIS_CHECK(cuvis_session_file_load(factoryDir, &sessionFile));
    CUVIS_CHECK(cuvis_calib_create_from_session_file(sessionFile, &calib));
    cuvis_session_file_free(&sessionFile);
  }
  else
  {
    fprintf(stderr, "Unrecognized file format: %s\n", factoryDir);
    return -1;
  }

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
  printf("initialize processing context...\n");
  CUVIS_CHECK(cuvis_proc_cont_create_from_calib(calib, &procCont));

  /*
  * Acquisition Context
  * The *Acquisition Context* is your interface to control the camera and all aspects of the data acquisition.
  *
  * Initialize it using a *SessionFile* object, then set the recording parameters and start an acquisition.
  * As soon as the **AcquisitionContext** is created, it will try to establish a connection with the camera.
  *
  * Here, the "Software" operation mode is used to enable data acquisition using a software trigger.
  * This is also called snapshot mode.
  *
  * Please note :
  * The *AcquisitionContext* will **only** connect to the **exact** camera of the same serial number matching the calibration file!
  * All other cameras / devices are ignored.
  */
  printf("initialize acquisition context...\n");
  CUVIS_CHECK(cuvis_acq_cont_create_from_calib(calib, &acqCont));

  printf("initialize measurement exporter...\n");
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



  strcpy(general_settings.export_dir, recDir);

  // Cube exporter specific settings
  CUVIS_EXPORT_CUBE_SETTINGS cube_settings = {
      session_merge_mode_Default, // CUVIS_SESSION_MERGE_MODE merge_mode
      1,                          // CUVIS_INT allow_overwrite
      0,                          // CUVIS_INT allow_drop: drop frames when the writer cannot keep up
      1,                          // CUVIS_INT allow_session_file: write a .cu3s file
      1,                          // CUVIS_INT allow_info_file: write a frame list next to it
      OperationMode_Software,     // CUVIS_OPERATION_MODE operation_mode
      0.0,                        // double fps: only for OperationMode_Internal
      2,                          // CUVIS_INT soft_limit: buffer size for reordering frames
      4,                          // CUVIS_INT hard_limit: max buffered frames
      10000,                      // CUVIS_INT max_buftime: max time a frame is buffered [ms]
      0,                          // CUVIS_INT full_export: include the cube
  };

  // Settings for worker (processing pipeline)
  CUVIS_WORKER_SETTINGS worker_settings = {
      10, // CUVIS_SIZE input_queue_size
      2,  // CUVIS_SIZE mandatory_queue_size
      2,  // CUVIS_SIZE supplementary_queue_size
      5,  // CUVIS_SIZE output_queue_size
      0,  // CUVIS_INT can_skip_measurements
      0,  // CUVIS_INT can_skip_supplementary_steps
      0,  // CUVIS_INT can_drop_results
  };

  cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings);

  printf("initialize processing pipeline worker...\n");
  CUVIS_WORKER worker;
  cuvis_worker_create(&worker, worker_settings);

  printf("waiting for camera to become online...\n");
  while (1)
  {
    CUVIS_HARDWARE_STATE state = hardware_state_offline;
    cuvis_acq_cont_get_state(acqCont, &state);

    if (state == hardware_state_online)
    {
      printf("\ncamera online\n");
      break;
    }
    if (state == hardware_state_partially_online)
    {
      printf("\ncamera partially online\n");
      break;
    }

#ifdef WIN32
    Sleep(1000);
#else
    usleep(1000000);
#endif
    printf(".");
    fflush(stdout);
  }

  printf("camera components details:\n");
  CUVIS_INT compCount;
  CUVIS_CHECK(cuvis_acq_cont_get_component_count(acqCont, &compCount));
  for (int compIdx = 0; compIdx < compCount; compIdx++)
  {
    CUVIS_INT online;
    CUVIS_COMPONENT_INFO cinfo = {0};

    CUVIS_CHECK(cuvis_acq_cont_get_component_info(acqCont, compIdx, &cinfo));
    CUVIS_CHECK(cuvis_comp_online_get(acqCont, compIdx, &online));
    printf(" - component '%s' is ", cinfo.displayname);
    if (online != 0)
    {
      printf("online\n");
    }
    else
    {
      printf("offline\n");
    }
    printf(" -- info:        %s\n", cinfo.sensorinfo);
    printf(" -- use:         %s\n", cinfo.userfield);
    printf(" -- pixelformat: %s\n", cinfo.pixelformat);
  }

  printf("configuring camera components...\n");
  // Configure exposure / integration time and software trigger
  CUVIS_CHECK(cuvis_acq_cont_integration_time_set(acqCont, exposure_ms));
  CUVIS_CHECK(cuvis_acq_cont_operation_mode_set(acqCont, OperationMode_Software));

  // Assign contexts to worker
  CUVIS_CHECK(cuvis_worker_set_acq_cont(worker, acqCont));
  CUVIS_CHECK(cuvis_worker_set_proc_cont(worker, procCont));
  CUVIS_CHECK(cuvis_worker_set_exporter(worker, cube_exporter));

  /*
  * Capturing a Measurement with Software Trigger(Single Snapshot)
  * Using the `capture()` method, a single measurement is initiated.
  * Taking a snapshot requires some time, so, to prevent the call to `capture()` from blocking execution, an *AsyncMesu* is returned.
  * To await the completion of the snapshot, use the `get()` method on the *AsyncMesu*.
  */
  printf("start recording now\n");
  CUVIS_CHECK(cuvis_worker_start(worker));

  for (int k = 0; k < nrImages; k++)
  {
    printf("trigger image nr. %d/%d\n", k + 1, nrImages);
    CUVIS_CHECK(cuvis_acq_cont_capture_async(acqCont, NULL));

    printf("waiting for processing...\n");
    CUVIS_CHECK(cuvis_worker_get_next_result(worker, NULL, NULL, exposure_ms + 2000));
  }
  CUVIS_CHECK(cuvis_worker_stop(worker));
  printf("done. cleaning up...\n");

  cuvis_exporter_free(&cube_exporter);
  cuvis_proc_cont_free(&procCont);
  cuvis_acq_cont_free(&acqCont);
  cuvis_calib_free(&calib);
  cuvis_worker_free(&worker);

  cuvis_shutdown();
  printf("finished.\n");
}
