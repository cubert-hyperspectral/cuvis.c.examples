#include "cuvis.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
* Process a fresh Measurement from the SessionFile with the current references and export it to out_dir/name.
* Returns status_ok on success, for use with CUVIS_CHECK.
*/
static int process_and_export(CUVIS_SESSION_FILE sess, CUVIS_PROC_CONT procCont, const char* out_dir, const char* name)
{
  CUVIS_MESU mesu;
  CUVIS_EXPORTER cube_exporter;

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings;
  memset(&general_settings, 0, sizeof(general_settings));
  snprintf(general_settings.export_dir, CUVIS_MAXBUF, "%s/%s", out_dir, name);
  strcpy(general_settings.pansharpening_settings.channel_selection, "all");
  general_settings.pansharpening_settings.spectra_multiplier = 1;

  CUVIS_EXPORT_CUBE_SETTINGS cube_settings;
  memset(&cube_settings, 0, sizeof(cube_settings));
  cube_settings.merge_mode = session_merge_mode_Default;
  cube_settings.allow_overwrite = 1;
  cube_settings.allow_session_file = 1;
  cube_settings.operation_mode = OperationMode_Software;
  cube_settings.soft_limit = 20;
  cube_settings.hard_limit = 100;
  cube_settings.max_buftime = 10000;
  cube_settings.full_export = 1; // include the reflectance cube in the export

  printf("processing to reflectance with %s\n", name);
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_get_mesu(sess, 0, session_item_type_frames_no_gaps, &mesu));
  CUVIS_CHECK(cuvis_proc_cont_apply(procCont, mesu));
  CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings, cube_settings));
  CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
  cuvis_exporter_free(&cube_exporter);
  cuvis_measurement_free(&mesu);
  return status_ok;
}

int main(int argc, char* argv[])
{
  /*
  * Reprocess a measurement with reference spectra
  *
  * Cuvis 3.6 adds two spectrum reference types to the processing context:
  *   - Reference_TargetSpectrum: Reflectance of the white target (fractions, 1.0 = 100%). Default without it: 100% at every wavelength, or the white_target_reflectivity_percentage setting.
  *   - Reference_WhiteSpectrum: Averaged white spectrum in dark-corrected sensor counts. Replaces the White reference measurement.
  *
  * The measurement is processed to reflectance with a target spectrum, then with a white spectrum.
  * Each result is exported as a SessionFile including the cube.
  *
  * Used principles:
  *   - *SessionFile* to load a recorded measurement and its references
  *   - *ProcessingContext* with `cuvis_proc_cont_set_reference_target_spectrum` and `cuvis_proc_cont_set_reference_white_spectrum`
  *   - *CubeExporter* to save the results
  *
  * Prerequisites to running this example:
  *   - Have a recorded measurement in *SessionFile* format (.cu3s) that contains Dark and White references *or* downloaded the provided [demo data](https://cloud.cubert-gmbh.de/s/SDKSampleData)
  *   - Have the Cuvis SDK 3.6 or newer installed
  *
  *   Run properties
  *   - "path/to/settings" "path/to/measurement/single.cu3s" "OutputFolderName"
  */
  if (argc != 4)
  {
    printf("Too few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("measurement file with Dark and White references (.cu3s)\n");
    printf("Name of output directory\n");

    return -1;
  }

  char* const userSettingsDir = argv[1];
  char* const measurementLoc = argv[2];
  char* const outDir = argv[3];

  printf("Example 06 reference spectra\n");
  printf("user Settings Dir: %s\n", userSettingsDir);
  printf("measurement file (.cu3s): %s\n", measurementLoc);
  printf("output Dir: %s\n", outDir);

  CUVIS_SESSION_FILE sess;
  CUVIS_PROC_CONT procCont;
  CUVIS_PROC_CONT darkSubProcCont;
  CUVIS_MESU white;
  CUVIS_IMBUFFER white_cube;
  CUVIS_SENSOR_INFO image_info;
  CUVIS_MESU_METADATA white_data;
  uint32_t x;
  uint32_t y;
  uint32_t chn;

  printf("loading settings... \n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_info, NULL));

  // The processing context loads the Dark and White references stored in the SessionFile
  printf("Load processing context...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_load(measurementLoc, &sess));
  CUVIS_CHECK(cuvis_proc_cont_create_from_session_file(sess, 1, &procCont));
  CUVIS_PROC_ARGS args;
  args.processing_mode = Cube_Reflectance;
  args.allow_recalib = 0;
  CUVIS_CHECK(cuvis_proc_cont_set_args(procCont, args));

  // Dark Subtract cube of the White reference measurement: source of the white spectrum below
  CUVIS_CHECK(cuvis_proc_cont_get_reference(procCont, &white, Reference_White));
  CUVIS_CHECK(cuvis_proc_cont_create_from_session_file(sess, 1, &darkSubProcCont));
  args.processing_mode = Cube_DarkSubtract;
  CUVIS_CHECK(cuvis_proc_cont_set_args(darkSubProcCont, args));
  CUVIS_CHECK(cuvis_proc_cont_apply(darkSubProcCont, white));
  CUVIS_CHECK(cuvis_measurement_get_data_image(white, CUVIS_MESU_CUBE_KEY, &white_cube));
  if (white_cube.format != imbuffer_format_uint16)
  {
    printf("16 bit cube required for this example\n");
    return -1;
  }

  /*
  * **Target Spectrum**
  *   - Values are fractions (1.0 = 100%), wavelengths in nm.
  *   - The spectrum must cover the cube's entire wavelength range, otherwise processing fails.
  *   - The SDK interpolates linearly between the points; they do not need to match the cube's wavelengths.
  *   - For a constant reflectivity, e.g. 95% (the constant reflectivity option of CuvisNEXT),
  *     set two points spanning the cube's range: the cube's first and last wavelength, values {0.95, 0.95}.
  *     For all processing, the cuvis.settings property white_target_reflectivity_percentage (default 100) sets it; a target spectrum overrides it.
  */
  // Example certificate of a white target (illustrative values): wavelength [nm] and reflectance as fraction
  const float certificate_wavelengths[] = {300, 350, 400, 450, 500, 600, 700, 800, 900, 1000, 1100};
  const float certificate_reflectance[] = {0.80f, 0.86f, 0.91f, 0.94f, 0.95f, 0.96f, 0.96f, 0.96f, 0.95f, 0.94f, 0.93f};

  CUVIS_TARGET_SPECTRUM target;
  target.wavelengths = certificate_wavelengths;
  target.values = certificate_reflectance;
  target.count = 11;
  CUVIS_CHECK(cuvis_proc_cont_set_reference_target_spectrum(procCont, &target));

  CUVIS_CHECK(process_and_export(sess, procCont, outDir, "TargetSpectrum"));

  CUVIS_CHECK(cuvis_proc_cont_clear_reference(procCont, Reference_TargetSpectrum));

  /*
  * **White Spectrum**
  *   - Dark-corrected sensor counts (uint16), e.g. averaged from a Dark Subtract cube, wavelengths in nm, covering the cube's entire wavelength range.
  *   - effective_bit_depth (1 to 16) is required: the bit depth of the sensor's pixel format, e.g. 12 for Mono12Packed.
  *   - integration_time [ms]: not used by default, so record the white spectrum with the same integration time as the measurements.
  *     With correct_ref_integration_time="true" in cuvis.settings, the reflectance is scaled by white spectrum / measurement integration time.
  *   - Channels with 0 counts (here the first channels, 350-358 nm) give no meaningful reflectance:
  *     setting such a spectrum logs a warning, and processing flags the cube as white-dark underflow.
  *   - White spectrum and White reference measurement are mutually exclusive: setting one clears the other.
  *
  * Limitation: A white spectrum is applied to the whole image equally and does not correct spatial effects,
  * like lens vignetting or uneven illumination, as a White reference measurement does.
  * This is an intended limitation of the current implementation, the first iteration of this feature:
  * some cameras allow exchanging the objective, which makes a fixed, calibrated spatial correction invalid in some cases.
  * For spatially accurate reflectance across the whole image, use a White reference measurement.
  */
  // Average the dark-corrected counts over a 40 x 40 pixel region in the image center
  double* white_counts = calloc(white_cube.channels, sizeof(double));
  float* white_wavelengths = malloc(white_cube.channels * sizeof(float));
  uint16_t* white_values = malloc(white_cube.channels * sizeof(uint16_t));
  const uint16_t* white16 = (const uint16_t*)white_cube.raw;
  for (y = white_cube.height / 2 - 20; y < white_cube.height / 2 + 20; y++)
  {
    for (x = white_cube.width / 2 - 20; x < white_cube.width / 2 + 20; x++)
    {
      for (chn = 0; chn < white_cube.channels; chn++)
      {
        white_counts[chn] += IMBUFFER_GET(white16, x, y, chn, white_cube) / 1600.0;
      }
    }
  }
  for (chn = 0; chn < white_cube.channels; chn++)
  {
    white_wavelengths[chn] = (float)white_cube.wavelength[chn];
    white_values[chn] = (uint16_t)rint(white_counts[chn]);
  }

  CUVIS_CHECK(cuvis_measurement_get_data_sensor_info(white, "IMAGE_info", &image_info));
  CUVIS_CHECK(cuvis_measurement_get_metadata(white, &white_data));

  CUVIS_SENSOR_SPECTRUM white_spectrum;
  white_spectrum.wavelengths = white_wavelengths;
  white_spectrum.values = white_values;
  white_spectrum.count = white_cube.channels;
  white_spectrum.effective_bit_depth = (uint16_t)strtol(image_info.pixel_format + strcspn(image_info.pixel_format, "0123456789"), NULL, 10);
  white_spectrum.integration_time = white_data.integration_time;
  CUVIS_CHECK(cuvis_proc_cont_set_reference_white_spectrum(procCont, &white_spectrum));

  CUVIS_CHECK(process_and_export(sess, procCont, outDir, "WhiteSpectrum"));

  free(white_counts);
  free(white_wavelengths);
  free(white_values);
  cuvis_measurement_free(&white);
  cuvis_proc_cont_free(&darkSubProcCont);
  cuvis_proc_cont_free(&procCont);
  cuvis_session_file_free(&sess);
  cuvis_shutdown();
  printf("finished.\n");
}
