<?php

/******************************************************************************
  Map
******************************************************************************/

/**
 * Returns map types.
 */
function datavizwiz_pane_map_types($type = NULL) {
  static $types;
  if (!isset($types)) {
    $types = array();
    foreach (module_implements('datavizwiz_pane_map_types') as $module) {
      $additions = module_invoke($module, 'datavizwiz_pane_map_types');
      foreach ($additions as $key => $addition) {
        $addition['module'] = $module;
        $types["{$module}_{$key}"] = $addition;
      }
    }
    drupal_alter('datavizwiz_pane_map_types', $types);
  }
  return array_key_exists($type, $types) ? $types[$type] : $types;
}

/**
 * Invoking our own hook.
 */
function datavizwiz_datavizwiz_pane_map_types() {
  // Google.
  $types['google'] = array(
    'name' => t('Google Maps'),
    'group' => 'google',
    'minzoom' => 0,
    'maxzoom' => 19,
    'terms' => t('<a href="https://developers.google.com/maps/terms" target="_blank">Google Maps Terms of Service</a> (if a federal agency, see <a href="http://www.google.com/intl/en-us/help/legalnotices_maps.html" target="_blank">U.S. Government Restricted Rights</a>)'),
    'render' => 'datavizwiz_pane_map_google_render',
  );

  // Open Street Maps.
  $types['open_street_maps'] = array(
    'name' => t('Open Street Maps'),
    'group' => 'open_street_maps',
    'minzoom' => 0,
    'maxzoom' => 19,
    'terms' => t('<a href="http://www.openstreetmap.org/copyright" target="_blank">OpenStreetMap Copyright and License</a>'),
  );

  // Mapbox map types.
  if (($cache = cache_get('datavizwiz:mapbox_types')) !== FALSE) {
    $mapbox_types = $cache->data;
  }
  else {
    $response = drupal_http_request('http://api.tiles.mapbox.com/v3/mapbox/maps.json');
    if ($response->code == 200) {
      $mapbox_types = json_decode($response->data);
      cache_set('datavizwiz:mapbox_types', $mapbox_types, 'cache', CACHE_TEMPORARY);
    }
  }
  if (!empty($mapbox_types)) {
    foreach ($mapbox_types as $mapbox_type) {
      $types[$mapbox_type->id] = array(
        'group' => 'mapbox',
        'name' => t('Mapbox') . ' - ' . $mapbox_type->name,
        'minzoom' => $mapbox_type->minzoom,
        'maxzoom' => $mapbox_type->maxzoom,
        'terms' => t('<a href="http://mapbox.com/tos/" target="_blank">MapBox Terms of Service</a>'),
        'render' => 'datavizwiz_pane_map_mapbox_render',
      );
    }
  }

  return $types;
}

/**
 * Pane form.
 */
function datavizwiz_pane_map_form($node, $pane = NULL) {
  // Build list of fields and replacement variables.
  $fields = $variables = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $settings) {
    $fields[$field_name] = !empty($settings['display_name']) ? $settings['display_name'] : $field_name;
    $variables[] = '<a href="#" class="datavizwiz-map-popup">@' . $field_name . '</a>';
  }

  $form['title_field'] = array(
    '#type' => 'select',
    '#title' => t('Title field'),
    '#description' => t('Field to display when hovering over a point on the map.'),
    '#options' => $fields,
    '#default_value' => !empty($pane->data['title_field']) ? $pane->data['title_field'] : NULL,
  );
  $form['link_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('Link title field to detail page'),
    '#description' => t('If checked, will turn title field into a link to the detail page. Disabled if popup content field is empty.'),
    '#default_value' => isset($pane->data['link_title']) ? $pane->data['link_title'] : TRUE,
  );
  $form['popup'] = array(
    '#type' => 'textarea',
    '#title' => t('Popup content'),
    '#description' => t('The content which pops up when clicking on the map markers. If empty, the markers will use hover behavior and show the Title field value.<br>Available replacement variables (these must be enabled for GeoJSON data export): !variables', array(
      '!variables' => join(', ', $variables),
    )),
    '#default_value' => !empty($pane->data['popup']) ? $pane->data['popup'] : '',
  );
  $form['popup_pan'] = array(
    '#type' => 'checkbox',
    '#title' => t('Pan map if popup is out of view'),
    '#description' => t('If unchecked, the map will not pan when a popup is clicked. If checked, the map will pan to make space so the popup can be as big as possible. This behavior only applies when the popup content field above has a value.'),
    '#default_value' => isset($pane->data['popup_pan']) ? $pane->data['popup_pan'] : FALSE,
    '#states' => array(
      'visible' => array(
        '#edit-data-popup' => array('filled' => TRUE),
      ),
    ),
  );

  $options = $settings = array();
  $maxzoom = 0;
  $types = datavizwiz_pane_map_types();
  $terms = '';
  foreach ($types as $key => $type) {
    $options[$key] = $type['name'];
    $settings['datavizwiz']['map_types'][$key] = array(
      'minzoom' => $type['minzoom'],
      'maxzoom' => $type['maxzoom'],
    );
    $maxzoom = max($maxzoom, $type['maxzoom']);
    if (!empty($type['terms'])) {
      $terms .= '<span id="map-terms-' . $key . '" class="element-hide">' . $type['terms'] . '</span>';
    }
  }
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Map type'),
    '#options' => $options,
    '#default_value' => !empty($pane->data['type']) ? $pane->data['type'] : '',
    '#description' => t('Please read and ensure your web site meets the Terms of Service for the selected map provider.<br>!terms', array(
      '!terms' => $terms,
    )),
    '#attached' => array(
      'js' => array(
        drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.map-form.js',
        array('data' => $settings, 'type' => 'setting'),
      ),
    ),
  );

  $form['center'] = array(
    '#type' => 'textfield',
    '#title' => t('Center'),
    '#description' => t('Initial coordinates for map center. Separate longitude and latitude by a comma, eg., "-98.35,39.50".'),
    '#default_value' => !empty($pane->data['center']) ? $pane->data['center'] : '-98.35,39.50',
  );
  $form['zoom_minimum'] = array(
    '#type' => 'select',
    '#title' => t('Minimum zoom level'),
    '#description' => t('The furthest the map can be zoomed out, where "0" is all the way zoomed out.'),
    '#options' => drupal_map_assoc(range(0, 20)),
    '#default_value' => isset($pane->data['zoom_minimum']) ? $pane->data['zoom_minimum'] : 0,
  );
  $form['zoom_maximum'] = array(
    '#type' => 'select',
    '#title' => t('Maximum zoom level'),
    '#description' => t('The furthest the map can be zoomed in.'),
    '#options' => drupal_map_assoc(range(0, 20)),
    '#default_value' => isset($pane->data['zoom_maximum']) ? $pane->data['zoom_maximum'] : $maxzoom,
  );
  $form['zoom_initial'] = array(
    '#type' => 'select',
    '#title' => t('Initial zoom level'),
    '#description' => t('The initial zoom level.'),
    '#options' => drupal_map_assoc(range(0, 20)),
    '#default_value' => isset($pane->data['zoom_initial']) ? $pane->data['zoom_initial'] : 4,
  );
  $form['max_radius'] = array(
    '#type' => 'textfield',
    '#title' => t('Max dot radius'),
    '#description' => t('The map will render a single dot for locations within close proximity to each other. You can set the maximum radius of this dot here. 20 is a good default. Use 0 to use no maximum radius.'),
    '#size' => 3,
    '#default_value' => !empty($pane->data['max_radius']) ? $pane->data['max_radius'] : 20,
  );
  $form['fill_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Fill color'),
    '#description' => t('The marker inside color.'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['fill_color']) ? $pane->data['fill_color'] : '66ccff',
  );
  $form['stroke_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Stroke color'),
    '#description' => t('The marker border color.'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['stroke_color']) ? $pane->data['stroke_color'] : '3366cc',
  );
  $form['select_fill_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Select fill color'),
    '#description' => t('The marker inside color (when selected).'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['select_fill_color']) ? $pane->data['select_fill_color'] : 'cccc33',
  );
  $form['select_stroke_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Select stroke color'),
    '#description' => t('The marker border color (when selected).'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['select_stroke_color']) ? $pane->data['select_stroke_color'] : 'cc66cc',
  );
  return $form;
}

/**
 * Render pane.
 */
function datavizwiz_pane_map_render($node, $pane) {
  // Add Javascript & CSS.
  $output['#attached']['js'] = array(
    drupal_get_path('module', 'datavizwiz') . '/openlayers/OpenLayers.js' => array(
      'group' => JS_LIBRARY,
      'preprocess' => FALSE,
    ),
    drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.map.js',
  );
  $output['#attached']['css'] = array(
    drupal_get_path('module', 'datavizwiz') . '/openlayers/theme/default/style.css',
  );

  $type = datavizwiz_pane_map_types($pane['data']['type']);

  // Save Javascript settings.
  $settings = array(
    'id' => $pane['html_id'],
    'type' => $pane['data']['type'],
    'group_type' => $type['group'],
    'layers' => NULL,
    'geojson_url' => url("node/$node->nid/datavizwiz/download/datavizwiz_geojson", array(
      'query' => array_merge(array_diff_key($_GET, array('q' => '')), array(
        'token' => drupal_get_token('datavizwiz-map'),
      )),
    )),
    'title_field' => $pane['data']['title_field'],
    'link_title' => $pane['data']['link_title'],
    'popup' => nl2br($pane['data']['popup']),
    'popup_pan' => $pane['data']['popup_pan'],
    'zoom_minimum' => (int) $pane['data']['zoom_minimum'],
    'zoom_maximum' => (int) $pane['data']['zoom_maximum'],
    'zoom_initial' => (int) $pane['data']['zoom_initial'],
    'center' => explode(',', $pane['data']['center']),
    'max_radius' => (int) $pane['data']['max_radius'],
    'stroke_color' => '#' . $pane['data']['stroke_color'],
    'fill_color' => '#' . $pane['data']['fill_color'],
    'select_stroke_color' => '#' . $pane['data']['select_stroke_color'],
    'select_fill_color' => '#' . $pane['data']['select_fill_color'],
  );

  // If map type defined a render callback, see if it wants to manipulate the
  // settings.
  if (!empty($type['render'])) {
    $settings = call_user_func($type['render'], $pane, $type, $settings);
  }

  $output['#attached']['js'][] = array(
    'data' => array('datavizwiz' => array('map' => array($settings))),
    'type' => 'setting',
  );
  $output['#markup'] = '<div class="datavizwiz-map-hoverbox"></div>';
  return $output;
}

/**
 * Google map render callback.
 */
function datavizwiz_pane_map_google_render($pane, $type, $settings) {
  drupal_add_js('http://maps.google.com/maps/api/js?v=3.6&sensor=false', array(
    'group' => JS_LIBRARY,
    'preprocess' => FALSE,
  ));
  drupal_add_css(drupal_get_path('module', 'datavizwiz') . '/openlayers/theme/default/google.css');
  return $settings;
}

/**
 * Mapbox render callback.
 */
function datavizwiz_pane_map_mapbox_render($pane, $type, $settings) {
  $mapbox_type = str_replace('datavizwiz_', '', $pane['data']['type']);
  $settings['layers'] = array(
    'http://a.tiles.mapbox.com/v3/' . $mapbox_type . '/${z}/${x}/${y}.png',
    'http://b.tiles.mapbox.com/v3/' . $mapbox_type . '/${z}/${x}/${y}.png',
    'http://c.tiles.mapbox.com/v3/' . $mapbox_type . '/${z}/${x}/${y}.png',
    'http://d.tiles.mapbox.com/v3/' . $mapbox_type . '/${z}/${x}/${y}.png',
  );
  return $settings;
}

/******************************************************************************
  Pie graph
******************************************************************************/

/**
 * Pane form.
 */
function datavizwiz_pane_pie_graph_form($node, $pane = NULL) {
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $settings) {
    $fields[$field_name] = !empty($settings['display_name']) ? $settings['display_name'] : $field_name;
  }
  $form['field'] = array(
    '#type' => 'select',
    '#title' => t('Field'),
    '#options' => $fields,
    '#default_value' => !empty($pane->data['field']) ? $pane->data['field'] : NULL,
  );
  $form['sort'] = array(
    '#type' => 'select',
    '#title' => t('Sort field'),
    '#options' => array(
      'count' => t('Number of rows'),
      'field' => t('Field value'),
    ),
    '#default_value' => !empty($pane->data['sort']) ? $pane->data['sort'] : 'count',
  );
  $form['direction'] = array(
    '#type' => 'select',
    '#title' => t('Sort direction'),
    '#options' => array(
      'ASC' => t('Ascending'),
      'DESC' => t('Descending'),
    ),
    '#default_value' => !empty($pane->data['direction']) ? $pane->data['direction'] : 'DESC',
  );
  $form['start_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Start color'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['start_color']) ? $pane->data['start_color'] : 'C6DBEF',
  );
  $form['end_color'] = array(
    '#type' => 'textfield',
    '#title' => t('End color'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['end_color']) ? $pane->data['end_color'] : '08306B',
  );
  return $form;
}

/**
 * Render pane.
 */
function datavizwiz_pane_pie_graph_render($node, $pane) {
  // Add Javascript.
  $output['#attached']['js'] = array(
    drupal_get_path('module', 'datavizwiz') . '/flot/jquery.flot.js' => array(
      'group' => JS_LIBRARY,
    ),
    drupal_get_path('module', 'datavizwiz') . '/flot/jquery.flot.pie.js' => array(
      'group' => JS_LIBRARY,
      'weight' => 1,
    ),
    drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.pie_graph.js',
  );

  $field_name = $pane['data']['field'];

  // Get grouped field values and number of rows.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't');
  $query->addField('t', $field_name, 'value');
  $query->addExpression('COUNT(*)', 'count');
  $query->groupBy($field_name);
  $query->orderBy($pane['data']['sort'] == 'field' ? $field_name : 'count', $pane['data']['direction']);
  datavizwiz_query_apply_filter($node, $query);
  $rows = $query->execute()->fetchAll();
  db_set_active();

  // Get Javascript settings for Flot.
  $data = array();
  $rowcount = count($rows);
  foreach ($rows as $key => $row) {
    $data[] = (object) array(
      'data' => $row->count,
      'color' => '#' . datavizwiz_color_blend($pane['data']['start_color'], $pane['data']['end_color'], 1 - $key / $rowcount),
      'label' => check_plain($row->value),
    );
  }
  $settings['datavizwiz']['pie_graph'][] = array(
    'id' => $pane['html_id'],
    'data' => $data,
  );
  $output['#attached']['js'][] = array(
    'data' => $settings,
    'type' => 'setting',
  );

  return $output;
}

/******************************************************************************
  Bar chart
******************************************************************************/

/**
 * Pane form.
 */
function datavizwiz_pane_bar_chart_form($node, $pane = NULL) {
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $settings) {
    $fields[$field_name] = !empty($settings['display_name']) ? $settings['display_name'] : $field_name;
  }
  $form['field'] = array(
    '#type' => 'select',
    '#title' => t('Field'),
    '#options' => $fields,
    '#default_value' => !empty($pane->data['field']) ? $pane->data['field'] : NULL,
  );
  $form['sort'] = array(
    '#type' => 'select',
    '#title' => t('Sort field'),
    '#options' => array(
      'count' => t('Number of rows'),
      'field' => t('Field value'),
    ),
    '#default_value' => !empty($pane->data['sort']) ? $pane->data['sort'] : 'count',
  );
  $form['direction'] = array(
    '#type' => 'select',
    '#title' => t('Sort direction'),
    '#options' => array(
      'ASC' => t('Ascending'),
      'DESC' => t('Descending'),
    ),
    '#default_value' => !empty($pane->data['direction']) ? $pane->data['direction'] : 'DESC',
  );
  $form['color'] = array(
    '#type' => 'textfield',
    '#title' => t('Bar color'),
    '#field_prefix' => '#',
    '#size' => 6,
    '#default_value' => !empty($pane->data['color']) ? $pane->data['color'] : '08519C',
  );
  return $form;
}

/**
 * Render pane.
 */
function datavizwiz_pane_bar_chart_render($node, $pane) {
  // Add Flot Javascript.
  $output['#attached']['js'] = array(
    drupal_get_path('module', 'datavizwiz') . '/flot/jquery.flot.js' => array(
      'group' => JS_LIBRARY,
    ),
    drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.bar_chart.js',
  );

  $field_name = $pane['data']['field'];

  // Get grouped field values and number of rows.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't');
  $query->addField('t', $field_name, 'value');
  $query->addExpression('COUNT(*)', 'count');
  $query->groupBy($field_name);
  $query->orderBy($pane['data']['sort'] == 'field' ? $field_name : 'count', $pane['data']['direction']);
  datavizwiz_query_apply_filter($node, $query);
  $rows = $query->execute()->fetchAll();
  db_set_active();

  // Set Javascript settings for Flot.
  $data = $ticks = array();
  foreach ($rows as $key => $row) {
    $data[] = array($key, $row->count);
    $ticks[] = array($key, check_plain($row->value));
  }
  $settings['datavizwiz']['bar_chart'][] = array(
    'id' => $pane['html_id'],
    'label' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
    'color' => '#' . $pane['data']['color'],
    'data' => $data,
    'ticks' => $ticks,
  );
  $output['#attached']['js'][] = array(
    'data' => $settings,
    'type' => 'setting',
  );

  return $output;
}

/******************************************************************************
  Text
******************************************************************************/

/**
 * Pane form - text.
 */
function datavizwiz_pane_text_form($node, $pane = NULL) {
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $settings) {
    $fields[$field_name] = !empty($settings['display_name']) ? $settings['display_name'] : $field_name;
  }
  $form['text'] = array(
    '#type' => 'text_format',
    '#title' => t('Text'),
    '#default_value' => !empty($pane->data['text']['value']) ? $pane->data['text']['value'] : NULL,
    '#format' => !empty($pane->data['text']['format']) ? $pane->data['text']['format'] : NULL,
  );
  return $form;
}

/**
 * Render pane.
 */
function datavizwiz_pane_text_render($node, $pane) {
  return array(
    '#markup' => check_markup($pane['data']['text']['value'], $pane['data']['text']['format']),
  );
}

/******************************************************************************
  Utilities
******************************************************************************/

/**
 * Returns result of blending two colors.
 *
 * @see http://www.gidnetwork.com/b-135.html
 *
 * @param $color1_hex
 *   A string containing the "from" color in hex format.
 * @param $color2_hex
 *   A string containing the "to" color in hex format.
 * @param $percent
 *   A float between 1 and 0 representing the blending percentage.
 */
function datavizwiz_color_blend($color1_hex, $color2_hex, $percent) {
  // Calculate blend amount.
  $blend = 100 - intval(min(1, max(0, $percent)) * 100);

  // Blend the colors.
  $color1['r'] = hexdec($color1_hex[0] . $color1_hex[1]);
  $color1['g'] = hexdec($color1_hex[2] . $color1_hex[3]);
  $color1['b'] = hexdec($color1_hex[4] . $color1_hex[5]);
  $color2['r'] = hexdec($color2_hex[0] . $color2_hex[1]);
  $color2['g'] = hexdec($color2_hex[2] . $color2_hex[3]);
  $color2['b'] = hexdec($color2_hex[4] . $color2_hex[5]);
  $add['r'] = ($color2['r'] - $color1['r']) / 100;
  $add['g'] = ($color2['g'] - $color1['g']) / 100;
  $add['b'] = ($color2['b'] - $color1['b']) / 100;
  $color1['r'] += intval($add['r'] * $blend);
  $color1['g'] += intval($add['g'] * $blend);
  $color1['b'] += intval($add['b'] * $blend);

  return sprintf('%02x%02x%02x', $color1['r'], $color1['g'], $color1['b']);
}
