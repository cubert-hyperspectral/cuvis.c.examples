#include "cuvis.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
  #include <Windows.h>
#else
  #include <unistd.h>
#endif

int main(int argc, char* argv[])
{
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
  char* const exposureString = argv[4]; //in ms
  char* const nrImagesString = argv[5];

  int exposure_ms = atoi(exposureString);
  int nrImages = atoi(nrImagesString);

  printf("Example 05 record single image\n");
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

  printf("load user settings...\n");
  CUVIS_CHECK(cuvis_init(userSettingsDir));
  cuvis_set_log_level(loglevel_info);

  printf("load calibration...\n");
  CUVIS_CHECK(cuvis_calib_create_from_path(factoryDir, &calib));

  printf("initialize processing context...\n");
  CUVIS_CHECK(cuvis_proc_cont_create_from_calib(calib, &procCont));

  printf("initialize acquisition context...\n");
  CUVIS_CHECK(cuvis_acq_cont_create_from_calib(calib, &acqCont));

  printf("initialize measurement exporter...\n");
  CUVIS_EXPORTER cube_exporter;

  // Export settings: General processing of measurements
  CUVIS_EXPORT_GENERAL_SETTINGS general_settings = {
      "", //initializer list only takes const char*, leave empty and modify afterwards.
      "all",
      1.0,
      0.0,
      pan_sharpening_interpolation_type_NearestNeighbor,
      pan_sharpening_algorithm_Noop,
      0,
      0};

  strcpy(general_settings.export_dir, recDir);

  // Cube exporter specific settings
  CUVIS_EXPORT_CUBE_SETTINGS cube_settings;
  cube_settings.allow_fragmentation = 0;
  cube_settings.allow_overwrite = 1;
  cube_settings.allow_session_file = 1;
  cube_settings.allow_info_file = 1;
  cube_settings.hard_limit = 4;
  cube_settings.soft_limit = 2;

  // Settings for worker (processing pipeline)
  CUVIS_WORKER_SETTINGS worker_settings;
  worker_settings.poll_interval = 5; //in ms
  worker_settings.worker_count = 1;
  worker_settings.worker_queue_hard_limit = 4;
  worker_settings.worker_queue_soft_limit = 2;
  worker_settings.can_drop = 0;

  cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings);

  printf("initialize processing pipeline worker...\n");
  CUVIS_WORKER worker;
  cuvis_worker_create(&worker, worker_settings);

  CUVIS_PROC_ARGS procArgs;
  procArgs.allow_recalib = 0;
  procArgs.processing_mode = Cube_Raw;

  printf("waiting for camera to become online...\n");
  while (1)
  {
    CUVIS_HARDWARE_STATE state;
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
  for (unsigned compIdx = 0; compIdx < compCount; compIdx++)
  {
    CUVIS_INT online;
    CUVIS_COMPONENT_INFO cinfo;

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

  printf("start recording now\n");

  for (int k = 0; k < nrImages; k++)
  {
    printf("trigger image nr. %d/%d\n", k + 1, nrImages);
    CUVIS_CHECK(cuvis_acq_cont_capture_async(acqCont, NULL));

    printf("waiting for processing...\n");
    CUVIS_CHECK(cuvis_worker_get_next_result(worker, NULL, NULL, exposure_ms + 2000));
  }
  printf("done. cleaning up...\n");

  cuvis_exporter_free(&cube_exporter);
  cuvis_proc_cont_free(&procCont);
  cuvis_acq_cont_free(&acqCont);
  cuvis_calib_free(&calib);
  cuvis_worker_free(&worker);

  printf("finished.\n");
}
