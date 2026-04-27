#include "cuvis.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  /*
  * Load and analyse a recorded measurement
  *
  * In this example an already recorded measurement (SessionFile .cu3s) is loaded.
  * The measurement's data and meta-data are accessed.
  *
  * Used principles:
  *   - *SessionFile* to load a recorded measurement
  *   - *Measurement* to access the SessionFiles data and meta-data
  *
  * Prerequisites to running this example:
  *   - Have a recorded measurement in *SessionFile* format (.cu3s) *or* downloaded the provided [demo data](https://drive.google.com/drive/folders/1Cjb0v_a2p1cCmhKH8w2OuRtnhXCJGz61?usp=sharing)
  *   - Have the Cuvis SDK installed
  *
  * Run properties
  *   - "path/to/settings" "path/to/file/file.cu3s"
  */
  if (argc != 3)
  {
    printf("To few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("sessionfile (.cu3s)\n");

    return -1;
  }

  char* const userSettingsDir = argv[1];
  char* const sessionLoc = argv[2];

  CUVIS_SESSION_FILE sess;
  CUVIS_MESU mesu1;
  unsigned chn;

  unsigned x;
  unsigned y;

  const uint16_t* cube16bit;
  //const uint16_t* info_ptr;

  printf("Example 02 load measurement\n");
  printf("\nUser Settings Dir: ");
  printf(userSettingsDir);
  printf("\nsessionfile (.cu3s): ");
  printf(sessionLoc);

  /*
  * **Settings**
  * Initialize the Cuvis SDK using a settings - directory
  * This is optional(all settings have defaults),
  * but enables you to optimize Cuvis' performance on your system using the settings
  * Your camera and the default Cuvis installation both provide these settings files
  */
  printf("\nloading user settings...\n");
  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_debug, NULL));
  CUVIS_CHECK(cuvis_set_log_level(loglevel_info));

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
  printf("loading session...\n");
  CUVIS_CHECK(cuvis_session_file_load(sessionLoc, &sess));

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
  printf("loading measurement...\n");
  CUVIS_CHECK(cuvis_session_file_get_mesu(sess, 0, session_item_type_frames_no_gaps, &mesu1));

  CUVIS_MESU_METADATA mesu_data;
  CUVIS_CHECK(cuvis_measurement_get_metadata(mesu1, &mesu_data));
  printf("data 1 %s %.2f ms mode=%d flags=%d\n", mesu_data.name, mesu_data.integration_time, mesu_data.processing_mode, mesu_data.measurement_flags);

  assert(mesu_data.processing_mode == Cube_Raw && "This example requires raw mode");

  CUVIS_IMBUFFER cube;
  CUVIS_CHECK(cuvis_measurement_get_data_image(mesu1, CUVIS_MESU_CUBE_KEY, &cube));
  CUVIS_IMBUFFER iminfo;
  cuvis_measurement_get_data_image(mesu1, CUVIS_MESU_CUBE_INFO_KEY, &iminfo);

  CUVIS_CHECK(cuvis_measurement_get_metadata(mesu1, &mesu_data));
  if (mesu_data.measurement_flags & CUVIS_MESU_FLAG_OVERILLUMINATED)
  {
    printf("-- is overilluminated --\n");
  }
  else
  {
    printf("-- is NOT overilluminated --\n");
  }

  if (mesu_data.measurement_flags & CUVIS_MESU_FLAG_POOR_REFERENCE)
  {
    printf("-- has poor reference --\n");
  }
  else
  {
    printf("-- has GOOD reference --\n");
  }

  printf("INFO cube No of channels: %d\n", iminfo.channels);
  printf("INFO cube width: %d\n", iminfo.width);
  printf("INFO cube height: %d\n", iminfo.height);
  printf("DATA cube width: %d\n", cube.width);
  printf("DATA cube height: %d\n", cube.height);
  printf("INFO cube type: %d\n", iminfo.format);
  printf("INFO cube bytes: %d\n", iminfo.bytes);
  printf("DATA cube type: %d\n", cube.format);
  printf("DATA cube bytes: %d\n", cube.bytes);

  assert(cube.format == imbuffer_format_uint16 && "16 bit cube required for this example");

  //reinterpret as uint16
  cube16bit = (const uint16_t*)(cube.raw);
  //info_ptr = (const uint16_t*)(iminfo.raw);

  x = 120;
  y = 200;

  assert(x < cube.width && "x index exceeds cube width");
  assert(y < cube.height && "x index exceeds cube width");

  printf("lambda [nm]; raw counts [au]; pixel info \n");
  for (chn = 0; chn < cube.channels; chn++)
  {
    // memory layout:
    //unsigned index = (y * cube.width + x) * cube.channels + chn;
    //uint16_t value = cube16bit[index];

    uint16_t value = IMBUFFER_GET(cube16bit, x, y, chn, cube);
    //only works with v3.X CUVIS data
    //auto pixel_info = IMBUFFER_GET(info_ptr, x, y, 0, iminfo);
    unsigned lambda = cube.wavelength[chn];

    printf(
        "%d; %d \n",
        //"%d; %d; %d \n",
        lambda,
        value
        //pixel_info);
    );
  }
  printf(" \n");

  //for (x = 0; x < cube.width; x++)
  //{
  //  for (y = 0; y < cube.height; y++)
  //  {
  // memory layout:
  //unsigned index = (y * cube.width + x) * cube.channels + chn;
  //uint16_t value = cube16bit[index];

  //auto pixel_info = IMBUFFER_GET(info_ptr, x, y, 0, iminfo);

  //printf("%d; ", pixel_info);
  //  }
  //  printf(" \n");
  //}

  cuvis_measurement_free(&mesu1);
  cuvis_session_file_free(&sess);
  cuvis_shutdown();
  printf("finished.\n");
}
