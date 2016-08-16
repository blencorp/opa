<?php

/**
 * Details page.
 */
function datavizwiz_detail($node) {
  // Get primary key values.
  $args = func_get_args();
  array_shift($args);

  // Return 404 if we have wrong number of primary key values.
  if (count($args) != count($node->datavizwiz['primary_keys'])) {
    return MENU_NOT_FOUND;
  }

  // Set primary key values in node.
  // TODO This should probably be set somewhere else and then passed as an
  // argument to theme_datavizwiz_details(), so that it can be rendered in
  // other places.
  $node->datavizwiz['primary_key_values'] = $args;

  // Render DataVizWiz Detail view mode.
  return node_view($node, 'datavizwiz_detail');
}

/**
 * Downloads a node's dataset in different formats.
 */
function datavizwiz_download($node, $format) {
  // Get download formats.
  $download_formats = datavizwiz_download_formats();

  // Allow modules to alter download access.
  $access = NULL;
  $context = array(
    'node' => clone $node,
    'format' => $format,
  );
  drupal_alter('datavizwiz_download_access', $access, $context);

  // If a module denies access, return access denied.
  if ($access === FALSE) {
    return MENU_ACCESS_DENIED;
  }

  // Return 404 if invalid or disabled format, and if no module has granted
  // access.
  if ((empty($node->datavizwiz['download_formats'][$format]) || empty($download_formats)) && empty($access)) {
    if (node_access('update', $node)) {
      drupal_set_message(t('This format is invalid or not enabled. Please <a href="!url">enable this format</a>.', array(
        '!url' => url("node/$node->nid/edit"),
      )), 'error');
    }
    return MENU_NOT_FOUND;
  }

  // Return 404 if invalid if no download fields are enabled.
  if (empty($node->datavizwiz['download_fields'])) {
    if (node_access('update', $node)) {
      drupal_set_message(t('No fields are enabled for data download. Please <a href="!url">enable some data fields</a>.', array(
        '!url' => url("node/$node->nid/datavizwiz/manage"),
      )), 'error');
    }
    return MENU_NOT_FOUND;
  }

  // Get enabled data fields.
  $fields = array_keys($node->datavizwiz['download_fields']);

  // Get results.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't');
  foreach ($fields as $field_name) {
    $query->addField('t', $field_name, "alias_$field_name");
  }
  datavizwiz_query_apply_filter($node, $query);
  $results = $query->execute()->fetchAll();
  db_set_active();

  // Remove field aliases.
  foreach ($results as $result) {
    datavizwiz_remove_aliases($result);
  }

  // Allow this to be accessed by cross-site domains.
  drupal_add_http_header('Access-Control-Allow-Origin', '*');
  drupal_add_http_header('Cache-Control', 'no-store, no-cache');

  // Render.
  if (!empty($download_formats[$format]['render'])) {
    // Require file if necessary.
    if (!empty($download_formats[$format]['file'])) {
      require_once drupal_get_path('module', $download_formats[$format]['module']) . '/' . $download_formats[$format]['file'];
    }
    $error = call_user_func($download_formats[$format]['render'], $node, $fields, $results);
  }

  // If we received an error, show it if the user can edit this node.
  if (!empty($error) && node_access('update', $node)) {
    drupal_set_message($error, 'error');
  }

  // If we got this far, nothing was rendered, so we'll issue a 404.
  return MENU_NOT_FOUND;
}

/**
 * Autocomplete callback for textfield filters.
 */
function datavizwiz_autocomplete($node, $field_name, $string = '') {
  $matches = array();
  if ($string) {
    // Lookup strings that match.
    db_set_active($node->datavizwiz['connection']);
    $query = db_select($node->datavizwiz['data_table'], 't');
    $query->addField('t', $field_name, "alias_$field_name");
    $query->condition("t.$field_name", '%' . db_like($string) . '%', 'LIKE');
    $query->orderBy("t.$field_name");
    $query->range(0, 10);
    $results = $query->execute()->fetchAll();
    db_set_active();

    // Remove field aliases.
    foreach ($results as $result) {
      datavizwiz_remove_aliases($result);
    }

    // Build array of matches, stripping HTML, and (display only) truncating
    // with an ellipsis.
    foreach ($results as $row) {
      $value = filter_xss($row->$field_name, array());
      $matches[truncate_utf8($value, 60, TRUE)] = truncate_utf8($value, 60, TRUE, TRUE);
    }
  }
  drupal_json_output($matches);
}
