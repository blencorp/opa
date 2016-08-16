<?php

/**
 * Exposes pane types.
 *
 * Modules can define their own pane types. Each type should return the
 * following information:
 * - title: The pane title.
 * - form: The function which returns additional form fields for adding/editing
 *   a pane.
 * - render: The function which returns the rendered pane.
 * - file: (optional) The file which contains your callback functions.
 */
function hook_datavizwiz_pane_types() {
  $types['mypane'] = array(
    'title' => t('My Pane Title'),
    'form' => 'mymodule_pane_mypane_form',
    'render' => 'mymodule_pane_mypane_render',
    'file' => 'includes/mypane.inc',
  );
  return $types;
}

/**
 * Alter pane types.
 */
function hook_datavizwiz_pane_types_alter(&$types) {
  $types['bar_chart']['render'] = 'mymodule_pane_bar_chart_render';
}

/**
 * Exposes download formats.
 *
 * Modules can define their own download formats. Each format should return the
 * following information:
 * - title: The download format title.
 * - render: The function which returns the download. It should set necessary
 *   headers and print the results. If this function returns a string error, it
 *   will be displayed to the user (if they can edit the node), and the page
 *   will return a 404 error.
 * - image: The image which will display in the download links.
 * - file: (optional) The file which contains your callback functions.
 */
function hook_datavizwiz_download_formats() {
  $formats['xls'] = array(
    'title' => t('Excel'),
    'render' => 'mymodule_download_format_xls',
    'image' => 'images/excel.png',
    'file' => 'includes/excel.inc',
  );
}

/**
 * Alter download formats.
 */
function hook_datavizwiz_download_formats_alter(&$formats) {
  $formats['xml']['render'] = 'mymodule_download_format_xml';
}

/**
 * Overwrites download format access.
 *
 * @param $access
 *   A mixed value variable which allows modules to allow (TRUE), deny (FALSE),
 *   or ignore (NULL) access to a download format.
 * @param $context
 *   An array containing the 'node' object and 'format' download format.
 */
function hook_datavizwiz_download_access_alter(&$access, $context) {
  if ($format == 'datavizwiz_geojson' && isset($_GET['token']) && drupal_valid_token($_GET['token'], 'datavizwiz-map')) {
    $access = TRUE;
  }
}

/**
 * Exposes map pane map types.
 *
 * Modules can extend the map pane to add their own map types. Each type should
 * return the following information:
 * - name: The map name.
 * - group: The map group (for example, "mapbox").
 * - minzoom: The minimum zoom level available for this map.
 * - maxzoom: The maximum zoom level available for this map.
 * - terms: (optional) Text which describe this map's terms of service.
 * - render: (optional) A function which will be called during rendering, and
 *   which can return additional Javascript settings.
 */
function hook_datavizwiz_pane_map_types() {
  $types['mapbox.my_tiles'] = array(
    'group' => 'mapbox',
    'name' => t('Mapbox - My Tiles'),
    'minzoom' => 0,
    'maxzoom' => 11,
    'terms' => t('<a href="http://mapbox.com/tos/" target="_blank">MapBox Terms of Service</a>'),
    'render' => 'my_module_map_render',
  );
  return $types;
}

/**
 * Alter map pane map types.
 */
function hook_datavizwiz_pane_map_types_alter(&$types) {
  $types['datavizwiz_google']['maxzoom'] = 11;
}
