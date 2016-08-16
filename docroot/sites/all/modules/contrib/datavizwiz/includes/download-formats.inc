<?php

/**
 * CSV download format.
 */
function datavizwiz_download_format_csv($node, $fields, $results) {
  drupal_add_http_header('Content-Type', 'text/csv; charset=UTF-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename="data.csv"');
  $stream = fopen('php://output', 'w');
  fputcsv($stream, $fields);
  foreach ($results as $result) {
    fputcsv($stream, (array) $result);
  }
  fclose($stream);
  exit;
}

/**
 * GeoJSON download format.
 */
function datavizwiz_download_format_geojson($node, $fields, $results) {
  // Find fields flagged as latitude and longitude.
  $latitude = $longitude = NULL;
  foreach ($fields as $field_name) {
    if ($node->datavizwiz['fields'][$field_name]['flag'] == 'latitude') {
      $latitude = $field_name;
    }
    if ($node->datavizwiz['fields'][$field_name]['flag'] == 'longitude') {
      $longitude = $field_name;
    }
  }

  // Latitude and longitude field mapping is required.
  if (empty($latitude) || empty($longitude)) {
    return t('No fields are flagged as latitude or longitude, or they are not enabled. Please <a href="!url">enable and flag these fields</a>.', array(
      '!url' => url("node/$node->nid/datavizwiz/manage"),
    ));
  }

  // Build data view.
  $features = array();
  foreach ($results as $result) {
    // Skip rows with no coordinates.
    if (empty($result->$latitude) || empty($result->$longitude)) {
      continue;
    }

    // Build feature data.
    $feature = array(
      'type' => 'Feature',
      'geometry' => array(
        'type' => 'Point',
        'coordinates' => array(
          (double) $result->$longitude,
          (double) $result->$latitude,
        ),
      ),
      'properties' => array(
        'fields' => $result,
      ),
    );

    // If pop post data is included, build the title and content. The title may
    // be linked to the detail page. The popup template needs to be replaced
    // with field values.
    $popup = '';
    if (!empty($_POST['title_field'])) {
      $title = $result->{$_POST['title_field']};
      if (!empty($_POST['link_title']) && !empty($_POST['popup_template'])) {
        $title = l($title, datavizwiz_detail_path($node, $result));
      }
      $popup .= '<div class="datavizwiz-map-popup-title">' . $title . '</div>';
    }
    if (!empty($_POST['popup_template'])) {
      $args = array();
      foreach ($result as $field_name => $value) {
        $args['@' . $field_name] = $value;
      }
      $popup .= '<div class="datavizwiz-map-popup-content">' . format_string($_POST['popup_template'], $args) . '</div>';
    }

    // Add popup content, if available.
    if (!empty($popup)) {
      $feature['properties']['popup'] = $popup;
    }

    $features[] = $feature;
  }

  drupal_add_http_header('Content-Type', 'application/json; charset=UTF-8');
  echo drupal_json_encode(array(
    'type' => 'FeatureCollection',
    'features' => $features,
  ));
  exit;
}

/**
 * JSON download format.
 */
function datavizwiz_download_format_json($node, $fields, $results) {
  drupal_add_http_header('Content-Type', 'application/json; charset=UTF-8');
  echo drupal_json_encode($results);
  exit;
}

/**
 * XML download format.
 */
function datavizwiz_download_format_xml($node, $fields, $results) {
  drupal_add_http_header('Content-Type', 'application/xml; charset=UTF-8');
  $xml = simplexml_load_string('<dataset></dataset>');
  foreach ($results as $result) {
    $data = $xml->addChild('data');
    foreach ($fields as $field) {
      $data->addChild($field, htmlspecialchars($result->$field));
    }
  }
  echo $xml->asXML();
  exit;
}
