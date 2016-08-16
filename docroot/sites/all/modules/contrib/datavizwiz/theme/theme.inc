<?php

/**
 * Preprocesses the panes.
 *
 * @param $variables
 *   An associative array containing:
 *   - node: The node.
 */
function template_preprocess_datavizwiz_panes(&$variables) {
  $node = $variables['node'];

  // Get pane types.
  $types = datavizwiz_pane_types();

  // Build menu and rendered panes.
  $variables['menu'] = $variables['panes'] = array();
  $delta = 0;
  foreach ($node->datavizwiz['panes'] as $pane) {
    // Render menu item.
    $variables['menu'][] = '<a href="#pane' . $delta . '">' . check_plain($pane['title']) . '</a>';

    // Generate pane HTML ID and index.
    $pane['html_id'] = drupal_html_id('datavizwiz-pane');
    $pane['delta'] = $delta;

    // Determine pane height style. If 0, use auto height.
    $pane['height_style'] = $pane['height'] > 0 ? "{$pane['height']}px" : 'auto';

    // Convert type into something usable in class name.
    $pane['type_class'] = str_replace('_', '-', $pane['type']);

    // Render pane content.
    $pane['render'] = '';
    if (!empty($types[$pane['type']]['render'])) {
      // Require file if necessary.
      if (!empty($types[$pane['type']]['file'])) {
        require_once drupal_get_path('module', $types[$pane['type']]['module']) . '/' . $types[$pane['type']]['file'];
      }
      $pane['render'] = call_user_func($types[$pane['type']]['render'], $node, $pane);
    }

    $variables['panes'][] = $pane;
    $delta++;
  }

  // Add Javascript, but weighted down a bit, so that pane type Javascript can
  // execute first and define the "datavizwiz-initialize" binding early enough.
  drupal_add_js(drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.panes.js', array(
    'weight' => 5,
  ));
}

/**
 * Preprocesses the download links.
 *
 * @param $variables
 *   An associative array containing:
 *   - node: The node.
 */
function template_preprocess_datavizwiz_download_links(&$variables) {
  $node = $variables['node'];
  $variables['links'] = array();

  // Skip if no download formats are available.
  if (empty($node->datavizwiz['download_formats']) || empty($node->datavizwiz['download_fields'])) {
    return;
  }

  // Get download formats.
  $download_formats = datavizwiz_download_formats();

  // Build list of download format links.
  $variables['links'] = array();
  foreach (array_filter($node->datavizwiz['download_formats']) as $format) {
    $image = theme('image', array(
      'path' => drupal_get_path('module', $download_formats[$format]['module']) . '/' . $download_formats[$format]['image'],
    ));
    $variables['links'][] = l($image, "node/$node->nid/datavizwiz/download/$format", array(
      'query' => array_diff_key($_GET, array('q' => '')),
      'html' => TRUE,
      'attributes' => array(
        'title' => t('Download this data set in !format format.', array(
          '!format' => $format,
        )),
      ),
    ));
  }
}

/**
 * Preprocesses the summary table.
 *
 * @param $variables
 *   An associative array containing:
 *   - node: The node.
 */
function template_preprocess_datavizwiz_summary_table(&$variables) {
  $node = $variables['node'];

  // Track the pager element, in case this is rendered more than once per page.
  static $pager_element = -1;
  $pager_element++;
  $variables['pager_element'] = $pager_element;

  $variables['header'] = array();
  $variables['rows'] = array();

  // Build header, using summary label if available; otherwise, display name.
  foreach (array_keys($node->datavizwiz['summary_field_order']) as $field_name) {
    $variables['header'][$field_name] = array(
      'data' => !empty($node->datavizwiz['fields'][$field_name]['summary_label'])
        ? check_plain($node->datavizwiz['fields'][$field_name]['summary_label'])
        : check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      'field' => !empty($node->datavizwiz['fields'][$field_name]['sortable']) ? "t.$field_name" : NULL,
    );
  }

  // Set default sort field and direction.
  if (!empty($node->datavizwiz['settings']['summary_sort_field'])) {
    $variables['header'][$node->datavizwiz['settings']['summary_sort_field']]['sort'] = $node->datavizwiz['settings']['summary_sort_order'];
  }

  // Get enabled fields.
  $enabled_fields = array_keys($node->datavizwiz['summary_field_order']);
  $fields = array_unique(array_merge($enabled_fields, $node->datavizwiz['primary_keys']));

  // Skip if there are no enabled fields.
  if (empty($enabled_fields)) {
    return;
  }

  // Build query, selecting only enabled fields and primary key fields.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't')
    ->extend('PagerDefault')->element($variables['pager_element'])
    ->extend('TableSort');
  foreach ($fields as $field_name) {
    $query->addField('t', $field_name, "alias_$field_name");
  }
  $query->orderByHeader($variables['header']);
  $query->limit(isset($node->datavizwiz['settings']['summary_limit']) ? $node->datavizwiz['settings']['summary_limit'] : 50);
  datavizwiz_query_apply_filter($node, $query);
  $results = $query->execute()->fetchAll();
  db_set_active();

  // Remove field aliases.
  foreach ($results as $result) {
    datavizwiz_remove_aliases($result);
  }

  // Build table rows of enabled fields.
  foreach ($results as $result) {
    $row = array();
    foreach ($enabled_fields as $field_name) {
      // Field is flagged as currency.
      if ($node->datavizwiz['fields'][$field_name]['flag'] == 'currency') {
        $result->$field_name = '$' . number_format($result->$field_name, 2);
      }

      // Field is configured as link to the detail page.
      if (!empty($node->datavizwiz['fields'][$field_name]['detail_link'])) {
        $result->$field_name = l($result->$field_name, datavizwiz_detail_path($node, $result));
      }

      $row[] = $result->$field_name;
    }
    $variables['rows'][] = $row;
  }
}

/**
 * Preprocesses the details.
 *
 * @param $variables
 *   An associative array containing:
 *   - node: The node.
 *   - set_page_title: Whether or not to set the page title to the enabled title
 *     field.
 */
function template_preprocess_datavizwiz_details(&$variables) {
  $node = $variables['node'];

  $variables['meta']['url'] = url("node/$node->nid");
  $variables['details'] = array();

  // Set "back" link text.
  $variables['meta']['back_link_text'] = empty($node->datavizwiz['settings']['detail_back'])
    ? '&larr; ' . t('Back')
    : ($node->datavizwiz['settings']['detail_back'] == '<none>'
        ? ''
        : '&larr; ' . check_plain($node->datavizwiz['settings']['detail_back']));

  // Get primary key values, and enabled fields.
  $primary_keys = !empty($node->datavizwiz['primary_keys']) ? $node->datavizwiz['primary_keys'] : array();
  $primary_key_values = !empty($node->datavizwiz['primary_key_values']) ? $node->datavizwiz['primary_key_values'] : array();
  $enabled_fields = !empty($node->datavizwiz['detail_field_order']) ? array_keys($node->datavizwiz['detail_field_order']) : array();

  // Skip if we don't have the right number of primary values, or there are no
  // enabled fields.
  if (count($primary_key_values) != count($primary_keys)) {
    return;
  }

  // Skip if there are no enabled fields.
  if (empty($enabled_fields)) {
    if (node_access('update', $node)) {
      drupal_set_message(t('No fields are enabled to display on the detail page. Please <a href="!url">enable some</a>.', array(
        '!url' => url("node/$node->nid/datavizwiz/manage/detail-page"),
      )), 'error');
    }
    return;
  }

  // Get row of enabled detail field values where primary keys match.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't');
  foreach ($enabled_fields as $enabled_field) {
    $query->addField('t', $enabled_field, "alias_$enabled_field");
  }
  foreach (array_combine($primary_keys, $primary_key_values) as $field => $value) {
    $query->condition("t.$field", $value);
  }
  $result = $query->execute()->fetchObject();
  db_set_active();

  // Remove field aliases.
  datavizwiz_remove_aliases($result);

  // Render enabled fields.
  foreach ($enabled_fields as $field_name) {
    // Hide empty fields.
    if (!empty($node->datavizwiz['fields'][$field_name]['hide_empty'])) {
      // Skip blank strings.
      if ($result->$field_name == '') {
        continue;
      }
      // Skip 0 values if 0 is considered empty.
      else if ($node->datavizwiz['fields'][$field_name]['empty_zero'] && in_array($result->$field_name, array(0, '0'), TRUE)) {
        continue;
      }
    }

    // Field is flagged as currency.
    if ($node->datavizwiz['fields'][$field_name]['flag'] == 'currency') {
      $result->$field_name = '$' . number_format($result->$field_name, 2);
    }

    // If page title is enabled and field is flagged as the title field, store
    // it for later, and skip field so it doesn't appear again on the page.
    if ($variables['set_page_title'] && !empty($node->datavizwiz['fields'][$field_name]['title'])) {
      $page_title = $result->$field_name;
      $variables['meta']['title'] = $page_title;
      continue;
    }

    $classes = 'datavizwiz-detail-field';
    if (!empty($node->datavizwiz['fields'][$field_name]['wide_display'])) {
      $classes .= ' datavizwiz-detail-wide';
    }

    $variables['details'][] = array(
      'classes' => $classes,
      'field_name' => $field_name,
      'label' => !empty($node->datavizwiz['fields'][$field_name]['detail_label'])
        ? check_plain($node->datavizwiz['fields'][$field_name]['detail_label'])
        : check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      'display_name' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      'value' => $result->$field_name,
      'value_safe' => html_entity_decode(check_markup($result->$field_name)),
    );
  }

  // Set page title, if enabled.
  if ($variables['set_page_title']) {
    drupal_set_title(!empty($page_title) ? $page_title : $node->title);
  }
}
