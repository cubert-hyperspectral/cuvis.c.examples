#include "cuvis.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
  /*
  * Expot / Convert Cubert Measurements to different file formats
  *
  * This example provides information on the exporter classes and the file formats which they can convert measurements to.
  *
  * Used principles:
  *   - *SessionFile* as a source for measurements
  *   - *CubeExporter* for saving measurements
  *   - *TiffExporter* for exporting to TIFF format
  *   - *EnviExporter* for exporting to ENVI format
  *   - *ViewExporter* for exporting rendered views of the data
  *   - *UserPlugins* to define how a view is computed
  *
  * Prerequisites to running this example:
  *   - Have recorded a *SessionFile* (.cu3s) *or* downloaded the provided [demo data](https://drive.google.com/drive/folders/1Cjb0v_a2p1cCmhKH8w2OuRtnhXCJGz61?usp=sharing)
  *   - Have the Cuvis SDK installed
  *
  * Run properties
  *   - "path/to/settings" path/to/measurement/single.cu3s" "path/to/user/plugin/file.xml" "OutputFolderName"
  */
  if (argc != 5)
  {
    printf("To few Arguments! Please provide:\n");
    printf("user settings directory\n");
    printf("sessionfile (.cu3s)\n");
    printf("user plugin file (.xml)\n");
    printf("Name of export directory\n");
    fflush(stdout);

    return -1;
  }
  const char* userSettingsDir = argv[1];
  const char* sessionLoc = argv[2];
  const char* pluginLoc = argv[3];
  const char* exportDir = argv[4];

  printf("Example 04 export measurement\n");
  printf("User Settings Dir: ");
  printf(userSettingsDir);
  printf("\nsessionfile (.cu3s): ");
  printf(sessionLoc);
  printf("\nuser plugin file (.xml): ");
  printf(pluginLoc);
  printf("\nExport Dir: ");
  printf(exportDir);
  fflush(stdout);

  CUVIS_SESSION_FILE sess;
  CUVIS_MESU mesu;

  CUVIS_EXPORTER envi_exporter;
  CUVIS_EXPORTER single_tiff_exporter;

  CUVIS_EXPORT_TIFF_SETTINGS single_tiff_settings;

  CUVIS_EXPORTER multi_tiff_exporter;
  CUVIS_EXPORT_TIFF_SETTINGS multi_tiff_settings;

  CUVIS_EXPORTER view_exporter;
  CUVIS_EXPORT_VIEW_SETTINGS view_settings;

  CUVIS_EXPORTER cube_exporter;

  char* buffer = 0;
  long length;
  FILE* f;

  printf("\nloading user settings...\n");
  fflush(stdout);

  CUVIS_CHECK(cuvis_init(userSettingsDir, loglevel_debug, NULL));
#ifdef _DEBUG
  CUVIS_CHECK(cuvis_set_log_level(loglevel_debug));
#else
  CUVIS_CHECK(cuvis_set_log_level(loglevel_info));
#endif

  CUVIS_PANSHARPENING_SETTINGS ps_settings = {
      "all",                                             // CUVIS_CHAR channel_selection[CUVIS_MAXBUF];
      1.0,                                               // float spectra_multiplier;
      0.0,                                               // double pan_scale;
      pan_sharpening_interpolation_type_NearestNeighbor, // CUVIS_PAN_SHAPRENING_INTERPOLATION_TYPE pan_interpolation_type;
      pan_sharpening_algorithm_Noop,                     // CUVIS_PAN_SHAPRENING_ALGORITHM_TYPE pan_algorithm;
      0,                                                 // CUVIS_INT pre_pan_sharpen_cube;
      0,                                                 // CUVIS_INT add_pan;
  };

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings_envi = {
      //initializer list only takes const char*, leave empty and modify afterwards.
      "",          //CUVIS_CHAR export_dir[CUVIS_MAXBUF];
      0,           //CUVIS_INT add_fullscale_pan;
      0,           //CUVIS_INT permissive;
      ps_settings, //CUVIS_PANSHARPENING_SETTINGS pansharpening_settings;
  };

  /*
  * ENVI Exporter
  * For the *EnviExporter* only some basic settings are available.
  * This exporter will create two files per measurement: A `.hdr` file and a data file with the same name but without a file extension.
  * `export_dir` and `channel_selection` apply the same as for *TiffExportSettings* above.
  */
  char exportDirEnvi[CUVIS_MAXBUF];
  strcpy(exportDirEnvi, exportDir);
  strcat(exportDirEnvi, "/envi");
  strcpy(general_settings_envi.export_dir, exportDirEnvi);

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings_single = {
      //initializer list only takes const char*, leave empty and modify afterwards.
      "",          //CUVIS_CHAR export_dir[CUVIS_MAXBUF];
      0,           //CUVIS_INT add_fullscale_pan;
      0,           //CUVIS_INT permissive;
      ps_settings, //CUVIS_PANSHARPENING_SETTINGS pansharpening_settings;
  };
  char exportDirSingle[CUVIS_MAXBUF];
  strcpy(exportDirSingle, exportDir);
  strcat(exportDirSingle, "/single");
  strcpy(general_settings_single.export_dir, exportDirSingle);

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings_multi = {
      //initializer list only takes const char*, leave empty and modify afterwards.
      "",          //CUVIS_CHAR export_dir[CUVIS_MAXBUF];
      0,           //CUVIS_INT add_fullscale_pan;
      0,           //CUVIS_INT permissive;
      ps_settings, //CUVIS_PANSHARPENING_SETTINGS pansharpening_settings;
  };
  char exportDirMulti[CUVIS_MAXBUF];
  strcpy(exportDirMulti, exportDir);
  strcat(exportDirMulti, "/multi");
  strcpy(general_settings_multi.export_dir, exportDirMulti);

  CUVIS_EXPORT_GENERAL_SETTINGS general_settings_view = {
      //initializer list only takes const char*, leave empty and modify afterwards.
      "",          //CUVIS_CHAR export_dir[CUVIS_MAXBUF];
      0,           //CUVIS_INT add_fullscale_pan;
      0,           //CUVIS_INT permissive;
      ps_settings, //CUVIS_PANSHARPENING_SETTINGS pansharpening_settings;
  };
  char exportDirView[CUVIS_MAXBUF];
  strcpy(exportDirView, exportDir);
  strcat(exportDirView, "/view");
  strcpy(general_settings_view.export_dir, exportDirView);

  CUVIS_EXPORT_CUBE_SETTINGS cube_settings_session = {0, 1, 0, 1, 1, OperationMode_Software, 1.0, 64, 128, 60000};
  CUVIS_EXPORT_GENERAL_SETTINGS general_settings_session = {
      //initializer list only takes const char*, leave empty and modify afterwards.
      "",          //CUVIS_CHAR export_dir[CUVIS_MAXBUF];
      0,           //CUVIS_INT add_fullscale_pan;
      0,           //CUVIS_INT permissive;
      ps_settings, //CUVIS_PANSHARPENING_SETTINGS pansharpening_settings;
  };
  char exportDirSession[CUVIS_MAXBUF];
  strcpy(exportDirSession, exportDir);
  strcat(exportDirSession, "/session");
  strcpy(general_settings_session.export_dir, exportDirSession);

  printf("loading session ...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_load(sessionLoc, &sess));

  printf("loading measurement ...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_session_file_get_mesu(sess, 0, session_item_type_frames_no_gaps, &mesu));

  printf("creating envi exporter ...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_exporter_create_envi(&envi_exporter, general_settings_envi));
  printf(" done.\n");
  fflush(stdout);

  /*
  * TIFF Exporter
  * The *TiffExporter* is used to export hyperspectral cubes (or parts thereof) in *.tiff* (*.tif*) format. As the TIFF format is fairly flexible, three general modes are available:
  *   - MultiChannel: The cube is stored as a single image in a single file with each wavelength band as a seperate channel
  *   - MultiPage: The cube is stored as multiple single channel (monochrome) images in a single file. Each wavelength band is a separate "page" within the TIFF file.
  *   - Single: The cube is stored as a multiple single channel (monochrome) image in multiple files. Each wavelength band is a separate TIFF file.
  *
  * The TIFF export mode is set via the *TiffExportSettings* class when creating the *TiffExporter*. Here is an overview of further useful settings:
  *   - `export_dir`: The directory to export the measurements to
  *   - `channel_selection`: Select which wavelength bands are included in the export by their wavelength \[nm\] value. This is set via string using a selection syntax. Here are some valid examples:
  *   - Include only the channel at 400nm: "400"
  *   - Include channels at 400, 410 and 620nm: "400;410;620"
  *   - Include all channels from 400 to 500nm (both exclusive): "400-500"
  *   - Include the channels 400 to 500nm and channel at 600nm: "400-500;600"
  *   - Include all channels: "all"
  *   - Include the channels between 400 and 600nm in 20nm steps: "400:20:600"
  *   - `compression_mode`: Which compression scheme to use. To enable compression, set to `TiffCompressionMode.LZW`.
  *   - `format`: Which TIFF export mode to use. Default: `TiffFormat.MultiChannel`
  */
  printf("creating single tiff exporter ...\n");
  fflush(stdout);

  single_tiff_settings.compression_mode = tiff_compression_mode_None;
  single_tiff_settings.format = tiff_format_Single;
  CUVIS_CHECK(cuvis_exporter_create_tiff(&single_tiff_exporter, general_settings_single, single_tiff_settings));
  printf(" done.\n");
  fflush(stdout);

  printf("creating multi tiff exporter ...\n");
  fflush(stdout);

  multi_tiff_settings.compression_mode = tiff_compression_mode_None;
  multi_tiff_settings.format = tiff_format_MultiChannel;
  CUVIS_CHECK(cuvis_exporter_create_tiff(&multi_tiff_exporter, general_settings_multi, multi_tiff_settings));
  printf(" done.\n");
  fflush(stdout);

  /*
  * View Exporter
  * The *ViewExporter* enables rendering and exporting views of the cube data, ie. RGB, Color Infrared, or indices like NDVI.
  * The views are always RGB images.
  * How the view is rendered / computed, is described in using a *UserPlugin* - an XML file with a special syntax that describes data accesses and mathematical operations used to compute the view.
  * These files are described in more detail below.
  *
  * The *ViewExporter* is initialized with the class *ViewExportSettings*, like the other exporters.
  * Here is an overview of some useful settings:
  *    - `export_dir` and `channel_selection`: Same as for *TiffExportSettings* above
  *    - `userplugin`: Either the path to the *UserPlugin* `.xml` file or a valid XML string of a *UserPlugin*
  *    - `pan_failback`: Controls the behavior if no cube is available. If `True`, allows using the panchromatic or preview image to be used as a fallback output. Else, throws an exception instead.
  *
  * User Plugins
  * The *UserPlugin* is a XML schema definition for how a hyperspectral cube is accessed and processed to compute a regular RGB image from it.
  * You can find the XML schema in your Cuvis installation under `Cuvis/user/plugin/userplugin.xsd`
  *
  * A selection of plugins is provided with your installation of Cuvis under `Cuvis/user/plugin`. The plugins differ slightly between processing modes, as some may only be applicable to, e.g. reflectance data.
  * For further information on *UserPlugins*, their syntax and the available operators, please refer to the *Cuvis User Plug-Ins manual* PDF included with your Cuvis installation.
  */
  printf("creating view exporter ...\n");
  printf("loading plugin ...\n");
  fflush(stdout);

  f = fopen(pluginLoc, "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer)
    {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }
  if (!buffer)
  {
    printf("failed to load plugin file\n");
    fflush(stdout);

    return 0;
  }

  view_settings.userplugin = buffer;
  CUVIS_CHECK(cuvis_exporter_create_view(&view_exporter, general_settings_view, view_settings));
  printf(" done.\n");
  fflush(stdout);

  /*
  * Cube Exporter - Settings (SaveArgs)
  * When setting up a *CubeExporter* there are many settings to fine-tune the export process and define what gets saved.
  * The *SaveArgs* class is used to communicate the settings to the *CubeExporter*.
  *
  * Here is an overview of the most important attributes of *SaveArgs*:
  *    - `export_dir`: The directory to export the measurements to
  *    - `allow_overwrite`: Allow overwriting files in case of a name clash
  *    - `allow_fragmentation`: Start a new *SessionFile* for each measurement / Allow only one measurement per *SessionFile*
  *    - `allow_drop`: Allow the exporter to drop measurements if it cannot write to the disk fast enough
  *    - `allow_info_file`: Create an info file alongside the *SessionFile*. This file contains a list of all measurements in the *SessionFile* and also marks dropped measurements
  *    - `full_export`: Include the hyperspectral cube when saving to disk. This is **FALSE** by default! *Please note:* The cube can **always be recomputed** on demand from the data stored in the *SessionFile*. Saving the cube can make sense to speed up or allow access to cube data on systems with low or insufficient processing power. It does significantly increase the size of the *SessionFiles*, usually roughly 2x. Additionally, if the cube is included, the processing mode (RAW, Reflectance, ...) is preserved.
  */
  printf("creating cube exporter (session) ...\n");
  fflush(stdout);
  CUVIS_CHECK(cuvis_exporter_create_cube(&cube_exporter, general_settings_session, cube_settings_session));
  printf(" done.\n");
  fflush(stdout);

  printf("export envi...");
  fflush(stdout);

  CUVIS_CHECK(cuvis_exporter_apply(envi_exporter, mesu));
  printf(" done.\n");
  fflush(stdout);

  printf("export single tiff...");
  fflush(stdout);
  CUVIS_CHECK(cuvis_exporter_apply(single_tiff_exporter, mesu));
  printf(" done.\n");
  fflush(stdout);

  printf("export multi tiff...");
  fflush(stdout);

  CUVIS_CHECK(cuvis_exporter_apply(multi_tiff_exporter, mesu));
  printf(" done.\n");
  fflush(stdout);

  printf("export view...");
  fflush(stdout);

  CUVIS_CHECK(cuvis_exporter_apply(view_exporter, mesu));
  printf("done.\n");
  fflush(stdout);

  printf("export cube (session)...");
  fflush(stdout);

  CUVIS_CHECK(cuvis_exporter_apply(cube_exporter, mesu));
  printf("done.\n");
  fflush(stdout);

  cuvis_measurement_free(&mesu);
  cuvis_session_file_free(&sess);
  cuvis_exporter_free(&envi_exporter);
  cuvis_exporter_free(&single_tiff_exporter);
  cuvis_exporter_free(&multi_tiff_exporter);
  cuvis_exporter_free(&view_exporter);
  cuvis_exporter_free(&cube_exporter);
  cuvis_shutdown();

  free(buffer);
  printf("finished.\n");
  fflush(stdout);

  return 0;
}
