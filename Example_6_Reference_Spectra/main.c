#include "cuvis.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
* Linearly interpolate a spectrum onto the cube's wavelengths,
* plus one 10nm step at both ends as a margin, so the spectrum safely covers the cube's full range.
* o_wavelengths and o_values must hold cube.channels + 2 elements.
*/
static void resample_to_cube(
    CUVIS_IMBUFFER cube, const double* src_wavelengths, const double* src_values, uint32_t src_count, float* o_wavelengths, double* o_values)
{
  uint32_t i;
  uint32_t j;
  o_wavelengths[0] = (float)cube.wavelength[0] - 10.0f;
  for (i = 0; i < cube.channels; i++)
  {
    o_wavelengths[i + 1] = (float)cube.wavelength[i];
  }
  o_wavelengths[cube.channels + 1] = (float)cube.wavelength[cube.channels - 1] + 10.0f;

  for (i = 0; i < cube.channels + 2; i++)
  {
    const double wl = o_wavelengths[i];
    if (wl <= src_wavelengths[0])
    {
      o_values[i] = src_values[0];
      continue;
    }
    if (wl >= src_wavelengths[src_count - 1])
    {
      o_values[i] = src_values[src_count - 1];
      continue;
    }
    for (j = 0; src_wavelengths[j + 1] < wl; j++)
    {
    }
    o_values[i] = src_values[j] + (wl - src_wavelengths[j]) / (src_wavelengths[j + 1] - src_wavelengths[j]) * (src_values[j + 1] - src_values[j]);
  }
}

/*
* Process a fresh Measurement from the SessionFile with the current references and export it to out_dir/name.
* Use a fresh Measurement for every reference configuration: processing with a reference spectrum stores it in the Measurement,
* and in Cuvis 3.6.0 a Measurement processed again after the spectrum was removed from the ProcessingContext keeps using it.
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
  *   - Reference_TargetSpectrum: Reflectance of the white target (fractions, 1.0 = 100%). Default without it: 100% at every wavelength.
  *   - Reference_WhiteSpectrum: Averaged white spectrum in raw sensor counts. Replaces the White reference measurement.
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
  CUVIS_PROC_CONT rawProcCont;
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

  // Raw cube of the White reference measurement: source of the white spectrum below and of the cube's wavelengths
  CUVIS_CHECK(cuvis_proc_cont_get_reference(procCont, &white, Reference_White));
  CUVIS_CHECK(cuvis_proc_cont_create_from_session_file(sess, 1, &rawProcCont));
  args.processing_mode = Cube_Raw;
  CUVIS_CHECK(cuvis_proc_cont_set_args(rawProcCont, args));
  CUVIS_CHECK(cuvis_proc_cont_apply(rawProcCont, white));
  CUVIS_CHECK(cuvis_measurement_get_data_image(white, CUVIS_MESU_CUBE_KEY, &white_cube));
  if (white_cube.format != imbuffer_format_uint16)
  {
    printf("16 bit cube required for this example\n");
    return -1;
  }

  const uint32_t count = white_cube.channels + 2;
  float* wavelengths = malloc(count * sizeof(float));
  double* values = malloc(count * sizeof(double));
  float* target_values = malloc(count * sizeof(float));
  uint16_t* white_values = malloc(count * sizeof(uint16_t));
  double* cube_wavelengths = malloc(white_cube.channels * sizeof(double));
  double* white_counts = calloc(white_cube.channels, sizeof(double));

  /*
  * **Target Spectrum**
  *   - Values are fractions (1.0 = 100%), wavelengths in nm.
  *   - The spectrum must cover the cube's entire wavelength range, otherwise processing fails.
  *   - Resample it onto the cube's wavelengths, so the SDK uses exactly the values you expect at every channel.
  *   - For a constant reflectivity, e.g. 95% (the constant reflectivity option of the Cuvis UI applications),
  *     set two points spanning the cube's range: wavelengths {first - 10, last + 10}, values {0.95, 0.95}.
  */
  // Example certificate of a white target (illustrative values): wavelength [nm] and reflectance as fraction
  const double certificate_wavelengths[] = {300, 350, 400, 450, 500, 600, 700, 800, 900, 1000, 1100};
  const double certificate_reflectance[] = {0.80, 0.86, 0.91, 0.94, 0.95, 0.96, 0.96, 0.96, 0.95, 0.94, 0.93};

  resample_to_cube(white_cube, certificate_wavelengths, certificate_reflectance, 11, wavelengths, values);
  for (chn = 0; chn < count; chn++)
  {
    target_values[chn] = (float)values[chn];
  }

  CUVIS_TARGET_SPECTRUM target;
  target.wavelengths = wavelengths;
  target.values = target_values;
  target.count = count;
  CUVIS_CHECK(cuvis_proc_cont_set_reference_target_spectrum(procCont, &target));

  CUVIS_CHECK(process_and_export(sess, procCont, outDir, "TargetSpectrum"));

  CUVIS_CHECK(cuvis_proc_cont_clear_reference(procCont, Reference_TargetSpectrum));

  /*
  * **White Spectrum**
  *   - Raw sensor counts (uint16), wavelengths in nm, covering the cube's entire wavelength range.
  *   - effective_bit_depth (1 to 16) is required: the bit depth of the sensor's pixel format, e.g. 12 for Mono12Packed.
  *   - integration_time [ms]: record the white spectrum with the same integration time as the measurements.
  *   - White spectrum and White reference measurement are mutually exclusive: setting one clears the other.
  *
  * Limitation: A white spectrum is applied to the whole image equally and does not correct spatial effects,
  * like lens vignetting or uneven illumination, as a White reference measurement does.
  * This is an intended limitation of the current implementation, the first iteration of this feature:
  * some cameras allow exchanging the objective, which makes a fixed, calibrated spatial correction invalid in some cases.
  * For spatially accurate reflectance across the whole image, use a White reference measurement.
  */
  // Average the raw counts over a 40 x 40 pixel region in the image center
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
    cube_wavelengths[chn] = white_cube.wavelength[chn];
  }
  resample_to_cube(white_cube, cube_wavelengths, white_counts, white_cube.channels, wavelengths, values);
  for (chn = 0; chn < count; chn++)
  {
    white_values[chn] = (uint16_t)rint(values[chn]);
  }

  CUVIS_CHECK(cuvis_measurement_get_data_sensor_info(white, "IMAGE_info", &image_info));
  CUVIS_CHECK(cuvis_measurement_get_metadata(white, &white_data));

  CUVIS_SENSOR_SPECTRUM white_spectrum;
  white_spectrum.wavelengths = wavelengths;
  white_spectrum.values = white_values;
  white_spectrum.count = count;
  white_spectrum.effective_bit_depth = (uint16_t)strtol(image_info.pixel_format + strcspn(image_info.pixel_format, "0123456789"), NULL, 10);
  white_spectrum.integration_time = white_data.integration_time;
  CUVIS_CHECK(cuvis_proc_cont_set_reference_white_spectrum(procCont, &white_spectrum));

  CUVIS_CHECK(process_and_export(sess, procCont, outDir, "WhiteSpectrum"));

  free(wavelengths);
  free(values);
  free(target_values);
  free(white_values);
  free(cube_wavelengths);
  free(white_counts);
  cuvis_measurement_free(&white);
  cuvis_proc_cont_free(&rawProcCont);
  cuvis_proc_cont_free(&procCont);
  cuvis_session_file_free(&sess);
  cuvis_shutdown();
  printf("finished.\n");
}
