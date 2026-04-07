#include "cuvis.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
  /*
  * Load and reprocess a recorded measurement
  *
  * In this example an already recorded measurement (SessionFile .cu3s) is loaded.
  * The measurement is reprocessed into different processing modes, explaining their differences.
  *
  * Used principles:
  *   - *SessionFile* to load a recorded measurement
  *   - *Measurement* to access the SessionFiles data and meta-data
  *   - *ProcessingContext* to generate hyperspectral cubes using different processing modes
  *
  * Prerequisites to running this example:
  *   - Have a recorded measurement in *SessionFile* format (.cu3s) *or* downloaded the provided [demo data](https://drive.google.com/drive/folders/1Cjb0v_a2p1cCmhKH8w2OuRtnhXCJGz61?usp=sharing)
  *   - Have a recorded White and Dark reference measurement *or* use the [demo data](https://drive.google.com/drive/folders/1Cjb0v_a2p1cCmhKH8w2OuRtnhXCJGz61?usp=sharing)
  *   - Have the Cuvis SDK installed
  *
  *   Run properties
  *   - "path/to/settings" path/to/measurement/single.cu3s" "path/to/dark/file.cu3s" "path/to/white/file.cu3s" "path/to/distance/file.cu3s" "OutputFolderName"
  */
  if (argc != 7)
  {
    printf("To few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("measurement file (.cu3s)\n");
    printf("dark file (.cu3s)\n");
    printf("white file (.cu3s)\n");
    printf("distance file (.cu3s)\n");
    printf("Name of output directory\n");

    return -1;
  }

  char* const userSettingsDir = argv[1];
  char* const measurementLoc = argv[2];
  char* const darkLoc = argv[3];
  char* const whiteLoc = argv[4];
  char* const distanceLoc = argv[5];
  char* const outDir = argv[6];

  printf("Example 02 reprocess measurement \n");
  printf("user Settings Dir: ");
  printf(userSettingsDir);
  printf("\nmeasurement file (.cu3s): ");
  printf(measurementLoc);
  printf("\ndark file (.cu3s): ");
  printf(darkLoc);
  printf("\nwhite file (.cu3s): ");
  printf(whiteLoc);
  printf("\ndistance file (.cu3s): ");
  printf(distanceLoc);
  printf("\noutput Dir: ");
  printf(outDir);

  CUVIS_SESSION_FILE sessMesu;
  CUVIS_SESSION_FILE sessDark;
  CUVIS_SESSION_FILE sessWhite;
  CUVIS_SESSION_FILE sessDistance;

  CUVIS_MESU mesu;
  CUVIS_MESU_METADATA mesu_data;
  CUVIS_MESU dark;
  CUVIS_MESU white;
  CUVIS_MESU distance;

  CUVIS_PROC_CONT procCont;

  CUVIS_INT is_capable;

  /*
  * **Settings**
  * Initialize the Cuvis SDK using a settings - directory
  * This is optional(all settings have defaults),
  * but enables you to optimize Cuvis' performance on your system using the settings
  * Your camera and the default Cuvis installation both provide these settings files
  */
  printf("\nloading settings... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_debug, NULL));

  /*
  * **SessionFile**
  * A SessionFile is a Cubert-proprietary container file format for storing measurement data from Cubert cameras.
  * It simplifies dealing with the calibration files, reference measurement and actual measurements by merging everything into a single file.
  *
  * A SessionFile can contain:
  *   - One or more *Measurements*
  *   - Reference Measurements (Dark, White, Distance, ...) (normally one per type)
  *   - Camera calibration file and Spectral Radiance calibration file
  *   - Meta-data about the recording settings (frame-rate, session name, etc.)
  */
  printf("loading sessionfiles... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_load(measurementLoc, &sessMesu));
  CUVIS_CHECK(cuvis_session_file_load(darkLoc, &sessDark));
  CUVIS_CHECK(cuvis_session_file_load(whiteLoc, &sessWhite));
  CUVIS_CHECK(cuvis_session_file_load(distanceLoc, &sessDistance));

  /*
  * **Measurement**
  * The *Measurement* class is the storage container for the actual hyperspectral data, along with more specific meta-data - things that can change from measurement to measurement (eg. integration time).
  *
  *  A Measurement can contain:
  *   - Multiple image data
  *   - A hyperspectral data cube
  *   - "Links" to reference measurements
  *   - Recording settings
  *   - Meta-data about the state of the camera
  *   - Meta-data about the software used to capture the measurement
  *   - Meta-data about the quality of the measurement
  *
  * Please note:
  * The hyperspectral cube is not always present.
  * Processing the measurement to generate the hyperspectral cube is a compute-intensive step and is thus only done on-demand!
  * By default, measurements are stored without the cube, but with all necessary data to generate it, to speed up saving and save on disk space (this shrinks measurements on average by about 50%)!
  */
  printf("loading measurement... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_get_mesu(sessMesu, 0, session_item_type_frames_no_gaps, &mesu));
  CUVIS_CHECK(cuvis_session_file_get_mesu(sessDark, 0, session_item_type_frames_no_gaps, &dark));
  CUVIS_CHECK(cuvis_session_file_get_mesu(sessWhite, 0, session_item_type_frames_no_gaps, &white));
  CUVIS_CHECK(cuvis_session_file_get_mesu(sessDistance, 0, session_item_type_frames_no_gaps, &distance));

  CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
  printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
  fflush(stdout);

  printf("Load processing context...\n");
  fflush(stdout);
  CUVIS_INT load_references = 1;
  CUVIS_CHECK(cuvis_proc_cont_create_from_session_file(sessMesu, load_references, &procCont));

  printf("Set references ...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_proc_cont_set_reference(procCont, dark, Reference_Dark));
  CUVIS_CHECK(cuvis_proc_cont_set_reference(procCont, white, Reference_White));
  CUVIS_CHECK(cuvis_proc_cont_set_reference(procCont, distance, Reference_Distance));

  printf("Prepare processing and export ...\n");
  CUVIS_PROC_ARGS args;
  args.processing_mode = Cube_Raw;
  args.allow_recalib = 0;
  CUVIS_CHECK(cuvis_proc_cont_is_capable(procCont, mesu, args, &is_capable));

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings = {
      "", // initializer list only takes const char*, leave empty and modify afterwards.
      "all",
      1,
      0.0,
      pan_sharpening_interpolation_type_NearestNeighbor,
      pan_sharpening_algorithm_Noop,
      0,
      0};

  CUVIS_EXPORT_CUBE_SETTINGS cube_settings;
  cube_settings.merge_mode = session_merge_mode_Default;
  cube_settings.allow_overwrite = 1;
  cube_settings.allow_session_file = 1;
  cube_settings.operation_mode = OperationMode_Internal;
  cube_settings.allow_info_file = 0;

  if (1 == is_capable)
  {
    printf("reprocess measurement to Cube_Raw mode...\n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));
    CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
    printf(" done. \n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
    printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
    fflush(stdout);

    char exportDirRAW[CUVIS_MAXBUF];
    strcpy(exportDirRAW, outDir);
    strcat(exportDirRAW, "/RAW");

    strcpy(general_settings.export_dir, exportDirRAW);
    CUVIS_EXPORTER cube_exporter;
    CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings));
    CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
    cuvis_exporter_free(&cube_exporter);
  }
  else
  {
    printf("Cannot process to Cube_Raw mode.\n");
    fflush(stdout);
  }

  args.processing_mode = Cube_DarkSubtract;
  CUVIS_CHECK(cuvis_proc_cont_is_capable(procCont, mesu, args, &is_capable));

  if (1 == is_capable)
  {
    printf("reprocess measurement to Cube_DarkSubtract mode...");
    fflush(stdout);
    CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));
    CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
    printf(" done. \n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
    printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
    fflush(stdout);

    char exportDirDS[CUVIS_MAXBUF];
    strcpy(exportDirDS, outDir);
    strcat(exportDirDS, "/DS");

    strcpy(general_settings.export_dir, exportDirDS);
    CUVIS_EXPORTER cube_exporter;
    CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings));
    CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
    cuvis_exporter_free(&cube_exporter);
  }
  else
  {
    printf("Cannot process to Cube_DarkSubtract mode.\n");
    fflush(stdout);
  }

  args.processing_mode = Cube_Reflectance;
  CUVIS_CHECK(cuvis_proc_cont_is_capable(procCont, mesu, args, &is_capable));

  if (1 == is_capable)
  {
    printf("reprocess measurement to Cube_Reflectance mode...\n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));
    CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
    printf(" done. \n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
    printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
    fflush(stdout);

    char exportDirREF[CUVIS_MAXBUF];
    strcpy(exportDirREF, outDir);
    strcat(exportDirREF, "/REF");

    strcpy(general_settings.export_dir, exportDirREF);
    CUVIS_EXPORTER cube_exporter;
    CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings));
    CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
    cuvis_exporter_free(&cube_exporter);
  }
  else
  {
    printf("Cannot process to Cube_Reflectance mode.\n");
    fflush(stdout);
  }

  args.processing_mode = Cube_SpectralRadiance;
  CUVIS_CHECK(cuvis_proc_cont_is_capable(procCont, mesu, args, &is_capable));

  if (1 == is_capable)
  {
    printf("reprocess measurement to Cube_SpectralRadiance mode...\n");
    fflush(stdout);
    CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));
    CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
    printf(" done. \n");
    fflush(stdout);

    CUVIS_CHECK(cuvis_measurement_get_metadata(mesu, &mesu_data));
    printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);
    fflush(stdout);

    char exportDirSPRAD[CUVIS_MAXBUF];
    strcpy(exportDirSPRAD, outDir);
    strcat(exportDirSPRAD, "/SPRAD");

    strcpy(general_settings.export_dir, exportDirSPRAD);
    CUVIS_EXPORTER cube_exporter;
    CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings));
    CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
    cuvis_exporter_free(&cube_exporter);
  }
  else
  {
    printf("Cannot process to Cube_SpectralRadiance mode.\n");
    fflush(stdout);
  }

  cuvis_proc_cont_free(&procCont);
  cuvis_measurement_free(&mesu);
  cuvis_measurement_free(&dark);
  cuvis_measurement_free(&white);
  cuvis_measurement_free(&distance);
  cuvis_session_file_free(&sessMesu);
  cuvis_session_file_free(&sessDark);
  cuvis_session_file_free(&sessWhite);
  cuvis_session_file_free(&sessDistance);
  cuvis_shutdown();
}
