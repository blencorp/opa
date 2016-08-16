<?php

/******************************************************************************
  Fields
******************************************************************************/

/**
 * Fields form.
 */
function datavizwiz_fields_form($form, $form_state, $node) {
  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Get schema of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);

  $form['primary_keys']['#tree'] = TRUE;
  $form['fields']['#tree'] = TRUE;

  // Build form from table fields.
  foreach ($schemas[$node->datavizwiz['data_table']]['fields'] as $field_name => $field_schema) {
    $form['fields'][$field_name]['display_name'] = array(
      '#type' => 'textfield',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['display_name']) ? $node->datavizwiz['fields'][$field_name]['display_name'] : $field_name,
      '#size' => 20,
    );
    $form['fields'][$field_name]['download'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['download']) ? $node->datavizwiz['fields'][$field_name]['download'] : FALSE,
    );
    $form['fields'][$field_name]['data'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['data']) ? $node->datavizwiz['fields'][$field_name]['data'] : TRUE,
    );
    $form['primary_keys'][$field_name] = array(
      '#type' => 'checkbox',
      '#default_value' => in_array($field_name, $node->datavizwiz['primary_keys']),
      '#disabled' => !empty($schemas[$node->datavizwiz['data_table']]['primary key']),
    );
    $form['fields'][$field_name]['flag'] = array(
      '#type' => 'select',
      '#options' => array(
        'currency' => t('Currency'),
        'latitude' => t('Latitude'),
        'longitude' => t('Longitude'),
      ),
      '#empty_option' => '',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['flag']) ? $node->datavizwiz['fields'][$field_name]['flag'] : '',
    );
  }

  // Submit.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Fields form - theme callback.
 */
function theme_datavizwiz_fields_form($variables) {
  // Get form.
  $form = $variables['form'];

  // Define select all checkbox Javascript.
  $javascript = '<input type="checkbox" onClick="jQuery(this).closest(\'table\').find(\'td.datavizwiz-NAME input\').attr(\'checked\', jQuery(this).is(\':checked\'));" />';

  // Select all checkboxes row.
  $rows[] = array(
    array('colspan' => 2),
    array('data' => str_replace('NAME', 'download', $javascript), 'class' => 'checkbox', 'header' => TRUE),
    array('data' => str_replace('NAME', 'data', $javascript), 'class' => 'checkbox', 'header' => TRUE),
    array('colspan' => 2),
  );

  // Build table rows.
  foreach (element_children($form['fields']) as $field_name) {
    $rows[] = array(
      $field_name,
      drupal_render($form['fields'][$field_name]['display_name']),
      array('data' => drupal_render($form['fields'][$field_name]['download']), 'class' => 'checkbox datavizwiz-download'),
      array('data' => drupal_render($form['fields'][$field_name]['data']), 'class' => 'checkbox datavizwiz-data'),
      array('data' => drupal_render($form['primary_keys'][$field_name]), 'class' => 'checkbox'),
      drupal_render($form['fields'][$field_name]['flag']),
    );
  }

  // Theme table.
  $output = theme('table', array(
    'header' => array(
      t('Field'),
      t('Display name'),
      array('data' => t('Download'), 'class' => 'checkbox'),
      array('data' => t('Data'), 'class' => 'checkbox'),
      array('data' => t('Primary'), 'class' => 'checkbox'),
      t('Flag'),
    ),
    'rows' => $rows,
  ));

  // Render remaining form items.
  return $output . drupal_render_children($form);
}

/**
 * Fields form - submit callback.
 */
function datavizwiz_fields_form_submit($form, $form_state) {
  // Update primary keys.
  db_update('datavizwiz')
    ->fields(array('primary_keys' => serialize(array_keys(array_filter($form_state['values']['primary_keys'])))))
    ->condition('nid', $form['#node']->nid)
    ->execute();

  // Update field settings.
  datavizwiz_merge_settings($form['#node'], $form_state['values']['fields']);
  drupal_set_message(t('Field settings updated.'));
}

/******************************************************************************
  Filter form
******************************************************************************/

/**
 * Filter form form.
 */
function datavizwiz_filter_form_form($form, $form_state, $node) {
  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $weight) {
    $fields[$field_name] = $schemas[$node->datavizwiz['data_table']]['fields'][$field_name];
  }

  // Build form from table fields.
  $form['fields']['#tree'] = TRUE;
  $default_sort = 0;
  foreach ($fields as $field_name => $field_schema) {
    $form['fields'][$field_name]['field_name'] = array(
      '#markup' => format_string('@display_name (!field_name)', array(
        '@display_name' => $node->datavizwiz['fields'][$field_name]['display_name'],
        '!field_name' => $field_name,
      )),
    );
    $form['fields'][$field_name]['filter'] = array(
      '#type' => 'select',
      '#options' => array(
        'textfield' => t('Text search'),
        'select' => t('Select'),
        'checkboxes' => t('Checkboxes'),
      ),
      '#empty_option' => '',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['filter']) ? $node->datavizwiz['fields'][$field_name]['filter'] : FALSE,
    );
    $form['fields'][$field_name]['filter_label'] = array(
      '#type' => 'textfield',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['filter_label']) ? $node->datavizwiz['fields'][$field_name]['filter_label'] : '',
      '#size' => 20,
      '#attributes' => array(
        'placeholder' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      ),
      '#states' => array(
        'invisible' => array(
          'select[name="fields[' . $field_name . '][filter]"]' => array('value' => ''),
        ),
      ),
    );
    $form['fields'][$field_name]['filter_key'] = array(
      '#type' => 'textfield',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['filter_key']) ? $node->datavizwiz['fields'][$field_name]['filter_key'] : '',
      '#size' => 20,
      '#attributes' => array(
        'placeholder' => $field_name,
      ),
      '#states' => array(
        'invisible' => array(
          'select[name="fields[' . $field_name . '][filter]"]' => array('value' => ''),
        ),
      ),
    );
    $form['fields'][$field_name]['filter_weight'] = array(
      '#type' => 'weight',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['filter_weight']) ? $node->datavizwiz['fields'][$field_name]['filter_weight'] : $default_sort++,
    );
    $form['fields'][$field_name]['#weight'] = $form['fields'][$field_name]['filter_weight']['#default_value'];
  }

  // Filter description field.
  $form['filter_description'] = array(
    '#type' => 'text_format',
    '#title' => t('Filter description'),
    '#description' => t('The text which appears above the form fields, inside the filter fieldset.'),
    '#default_value' => isset($node->datavizwiz['settings']['filter_description']['value']) ? $node->datavizwiz['settings']['filter_description']['value'] : '',
    '#format' => isset($node->datavizwiz['settings']['filter_description']['format']) ? $node->datavizwiz['settings']['filter_description']['format'] : NULL,
  );

  // Filter submit text field.
  $form['filter_submit'] = array(
    '#type' => 'textfield',
    '#title' => t('Submit button'),
    '#description' => t('The text which appears on the submit button.'),
    '#default_value' => isset($node->datavizwiz['settings']['filter_submit']) ? $node->datavizwiz['settings']['filter_submit'] : t('Filter'),
    '#required' => TRUE,
  );

  // Default sort field.
  $form['filter_reset'] = array(
    '#type' => 'textfield',
    '#title' => t('Reset link'),
    '#description' => t("The text which appears on the reset link, which appears after the form has been submitted, and resets the form to it's initial state."),
    '#default_value' => isset($node->datavizwiz['settings']['filter_reset']) ? $node->datavizwiz['settings']['filter_reset'] : t('Reset'),
    '#required' => TRUE,
  );

  // Submit.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Filter form form - theme callback.
 */
function theme_datavizwiz_filter_form_form($variables) {
  // Get form.
  $form = $variables['form'];

  // Build table rows, sorted by filter weight.
  $rows = array();
  foreach (element_children($form['fields'], TRUE) as $field_name) {
    $form['fields'][$field_name]['filter_weight']['#attributes']['class'][] = 'filter-table-order-weight';
    $rows[] = array(
      'data' => array(
        drupal_render($form['fields'][$field_name]['field_name']),
        drupal_render($form['fields'][$field_name]['filter']),
        drupal_render($form['fields'][$field_name]['filter_label']),
        drupal_render($form['fields'][$field_name]['filter_key']),
        drupal_render($form['fields'][$field_name]['filter_weight']),
      ),
      'class' => array('draggable'),
    );
  }

  // Add table drag for weight field.
  drupal_add_tabledrag('filter-table-order', 'order', 'sibling', 'filter-table-order-weight');

  // Theme table.
  $output = theme('table', array(
    'header' => array(
      t('Field'),
      t('Filter'),
      t('Label'),
      t('URL Key'),
      t('Weight'),
    ),
    'rows' => $rows,
    'attributes' => array('id' => 'filter-table-order'),
  ));

  // Render remaining form items.
  return $output . drupal_render_children($form);
}

/**
 * Filter form form - validate callback.
 */
function datavizwiz_filter_form_form_validate($form, $form_state) {
  foreach ($form_state['values']['fields'] as $field_name => $field_settings) {
    // Ensure that no filter key uses invalid characters.
    if (!empty($field_settings['filter_key']) && preg_match('/[^\w-]/', $field_settings['filter_key']) !== 0) {
      form_set_error('fields][company_name][filter_key', t('URL key can only use alphanumeric characters, underscores, and dashes.'));
    }

    // Ensure that no filter key uses a field name.
    else if ($field_settings['filter_key'] != $field_name && array_key_exists($field_settings['filter_key'], $form_state['values']['fields'])) {
      form_set_error('fields][company_name][filter_key', t('URL key conflicts with another field name.'));
    }
  }
}

/**
 * Filter form form - submit callback.
 */
function datavizwiz_filter_form_form_submit($form, $form_state) {
  // Update settings.
  datavizwiz_merge_settings($form['#node'], $form_state['values']['fields'], array(
    'filter_description' => $form_state['values']['filter_description'],
    'filter_submit' => $form_state['values']['filter_submit'],
    'filter_reset' => $form_state['values']['filter_reset'],
  ));

  drupal_set_message(t('Filter form settings updated.'));
}

/******************************************************************************
  Summary table
******************************************************************************/

/**
 * Summary table form.
 */
function datavizwiz_summary_table_form($form, $form_state, $node) {
  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $weight) {
    $fields[$field_name] = $schemas[$node->datavizwiz['data_table']]['fields'][$field_name];
  }

  // Build form from table summary fields.
  $form['fields']['#tree'] = TRUE;
  $sort_options = array();
  $default_sort = 0;
  foreach ($fields as $field_name => $field_schema) {
    $form['fields'][$field_name]['field_name'] = array(
      '#markup' => format_string('@display_name (!field_name)', array(
        '@display_name' => $node->datavizwiz['fields'][$field_name]['display_name'],
        '!field_name' => $field_name,
      )),
    );
    $form['fields'][$field_name]['summary'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['summary']) ? $node->datavizwiz['fields'][$field_name]['summary'] : FALSE,
    );
    $form['fields'][$field_name]['summary_label'] = array(
      '#type' => 'textfield',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['summary_label']) ? $node->datavizwiz['fields'][$field_name]['summary_label'] : '',
      '#size' => 20,
      '#attributes' => array(
        'placeholder' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      ),
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][summary]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['sortable'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['sortable']) ? $node->datavizwiz['fields'][$field_name]['sortable'] : FALSE,
      '#attributes' => array(
        'data-datavizwiz-key' => $field_name,
        'data-datavizwiz-value' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      ),
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][summary]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['detail_link'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['detail_link']) ? $node->datavizwiz['fields'][$field_name]['detail_link'] : FALSE,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][summary]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['summary_weight'] = array(
      '#type' => 'weight',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['summary_weight']) ? $node->datavizwiz['fields'][$field_name]['summary_weight'] : $default_sort++,
    );
    $form['fields'][$field_name]['#weight'] = $form['fields'][$field_name]['summary_weight']['#default_value'];

    // Include all fields as default sort field candidates.
    $sort_options[$field_name] = check_plain($node->datavizwiz['fields'][$field_name]['display_name']);
  }

  // Limit field.
  $form['summary_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit'),
    '#size' => 4,
    '#description' => t('Number of rows to show in summary page table. Enter "0" to display unlimited rows.'),
    '#default_value' => isset($node->datavizwiz['settings']['summary_limit']) ? $node->datavizwiz['settings']['summary_limit'] : 50,
    '#required' => TRUE,
  );

  // Default sort field.
  $form['summary_sort_field'] = array(
    '#type' => 'select',
    '#title' => t('Default sort field'),
    '#options' => array(NULL => '- ' . t('None') . '-') + $sort_options,
    '#description' => t('Field to sort results by initially. Only fields checked as "sortable" are eligible.'),
    '#default_value' => isset($node->datavizwiz['settings']['summary_sort_field']) ? $node->datavizwiz['settings']['summary_sort_field'] : NULL,
    '#attached' => array(
      'js' => array(
        drupal_get_path('module', 'datavizwiz') . '/js/datavizwiz.summary-table-form.js',
      ),
    ),
  );

  // Default sort field order.
  $form['summary_sort_order'] = array(
    '#type' => 'select',
    '#title' => t('Default sort direction'),
    '#options' => array(
      'asc' => t('Ascending'),
      'desc' => t('Descending'),
    ),
    '#description' => t('Initial sort direction.'),
    '#default_value' => isset($node->datavizwiz['settings']['summary_sort_order']) ? $node->datavizwiz['settings']['summary_sort_order'] : NULL,
  );

  // Submit.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Summary table form - theme callback.
 */
function theme_datavizwiz_summary_table_form($variables) {
  // Get form.
  $form = $variables['form'];

  // Build table rows, sorted by summary weight.
  $rows = array();
  foreach (element_children($form['fields'], TRUE) as $field_name) {
    $form['fields'][$field_name]['summary_weight']['#attributes']['class'][] = 'summary-table-order-weight';
    $rows[] = array(
      'data' => array(
        drupal_render($form['fields'][$field_name]['field_name']),
        array('data' => drupal_render($form['fields'][$field_name]['summary']), 'class' => 'checkbox'),
        drupal_render($form['fields'][$field_name]['summary_label']),
        array('data' => drupal_render($form['fields'][$field_name]['sortable']), 'class' => 'checkbox'),
        array('data' => drupal_render($form['fields'][$field_name]['detail_link']), 'class' => 'checkbox'),
        drupal_render($form['fields'][$field_name]['summary_weight']),
      ),
      'class' => array('draggable'),
    );
  }

  // Add table drag for weight field.
  drupal_add_tabledrag('summary-table-order', 'order', 'sibling', 'summary-table-order-weight');

  // Theme table.
  $output = theme('table', array(
    'header' => array(
      t('Field'),
      array('data' => t('Enabled'), 'class' => 'checkbox'),
      t('Label'),
      array('data' => t('Sortable'), 'class' => 'checkbox'),
      array('data' => t('Link to detail'), 'class' => 'checkbox'),
      t('Weight'),
    ),
    'rows' => $rows,
    'attributes' => array('id' => 'summary-table-order'),
    'empty' => t('No fields are configured to appear on the summary page. <a href="!url">Select some fields</a>.', array(
      '!url' => url("node/{$form['#node']->nid}/datavizwiz/manage"),
    )),
  ));

  // Render remaining form items.
  return $output . drupal_render_children($form);
}

/**
 * Summary table form - submit callback.
 */
function datavizwiz_summary_table_form_submit($form, $form_state) {
  // Update settings.
  datavizwiz_merge_settings($form['#node'], $form_state['values']['fields'], array(
    'summary_limit' => $form_state['values']['summary_limit'],
    'summary_sort_field' => $form_state['values']['summary_sort_field'],
    'summary_sort_order' => $form_state['values']['summary_sort_order'],
  ));

  drupal_set_message(t('Summary page field settings updated.'));
}

/******************************************************************************
  Detail page
******************************************************************************/

/**
 * Detail page form.
 */
function datavizwiz_detail_page_form($form, $form_state, $node) {
  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);
  $fields = array();
  foreach ($node->datavizwiz['fields'] as $field_name => $weight) {
    $fields[$field_name] = $schemas[$node->datavizwiz['data_table']]['fields'][$field_name];
  }

  // Add "none" radio.
  $form['no_title'] = array(
    '#type' => 'radio',
    '#return_value' => TRUE,
    '#title' => t('none'),
    '#parents' => array('title'),
    '#default_value' => TRUE,
  );

  // Build form from table detail fields.
  $form['fields']['#tree'] = TRUE;
  $default_sort = 0;
  foreach ($fields as $field_name => $field_schema) {
    $form['fields'][$field_name]['field_name'] = array(
      '#markup' => format_string('@display_name (!field_name)', array(
        '@display_name' => $node->datavizwiz['fields'][$field_name]['display_name'],
        '!field_name' => $field_name,
      )),
    );
    $form['fields'][$field_name]['detail'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['detail']) ? $node->datavizwiz['fields'][$field_name]['detail'] : FALSE,
    );
    $form['fields'][$field_name]['detail_label'] = array(
      '#type' => 'textfield',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['detail_label']) ? $node->datavizwiz['fields'][$field_name]['detail_label'] : '',
      '#size' => 20,
      '#attributes' => array(
        'placeholder' => check_plain($node->datavizwiz['fields'][$field_name]['display_name']),
      ),
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['title'] = array(
      '#type' => 'radio',
      '#return_value' => $field_name,
      '#parents' => array('title'),
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['title']) ? $node->datavizwiz['fields'][$field_name]['title'] : FALSE,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    // If any field has Title selected, de-select "none" radio.
    if ($form['fields'][$field_name]['title']['#default_value']) {
      $form['no_title']['#default_value'] = FALSE;
    }
    $form['fields'][$field_name]['wide_display'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['wide_display']) ? $node->datavizwiz['fields'][$field_name]['wide_display'] : FALSE,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['hide_empty'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['hide_empty']) ? $node->datavizwiz['fields'][$field_name]['hide_empty'] : FALSE,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['empty_zero'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['empty_zero']) ? $node->datavizwiz['fields'][$field_name]['empty_zero'] : FALSE,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['detail_weight'] = array(
      '#type' => 'weight',
      '#default_value' => isset($node->datavizwiz['fields'][$field_name]['detail_weight']) ? $node->datavizwiz['fields'][$field_name]['detail_weight'] : $default_sort++,
      '#states' => array(
        'visible' => array(
          'input[name="fields[' . $field_name . '][detail]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['fields'][$field_name]['#weight'] = $form['fields'][$field_name]['detail_weight']['#default_value'];
  }

  // Back link text.
  $form['detail_back'] = array(
    '#type' => 'textfield',
    '#title' => t('Back link text'),
    '#description' => t('The text for the "back" link. If "@none", it will not appear.', array('@none' => '<none>')),
    '#default_value' => isset($node->datavizwiz['settings']['detail_back']) ? $node->datavizwiz['settings']['detail_back'] : t('Back'),
  );

  // Submit.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Detail page form - theme callback.
 */
function theme_datavizwiz_detail_page_form($variables) {
  // Get form.
  $form = $variables['form'];

  // Add "none" radio row.
  $rows[] = array(
    array('colspan' => 3),
    array('data' => drupal_render($form['no_title']), 'class' => 'checkbox'),
    array('colspan' => 4),
  );

  // Build table rows, sorted by detail weight.
  foreach (element_children($form['fields'], TRUE) as $field_name) {
    $form['fields'][$field_name]['detail_weight']['#attributes']['class'][] = 'detail-page-order-weight';
    $rows[] = array(
      'data' => array(
        drupal_render($form['fields'][$field_name]['field_name']),
        array('data' => drupal_render($form['fields'][$field_name]['detail']), 'class' => 'checkbox'),
        drupal_render($form['fields'][$field_name]['detail_label']),
        array('data' => drupal_render($form['fields'][$field_name]['title']), 'class' => 'checkbox'),
        array('data' => drupal_render($form['fields'][$field_name]['wide_display']), 'class' => 'checkbox'),
        array('data' => drupal_render($form['fields'][$field_name]['hide_empty']), 'class' => 'checkbox'),
        array('data' => drupal_render($form['fields'][$field_name]['empty_zero']), 'class' => 'checkbox'),
        drupal_render($form['fields'][$field_name]['detail_weight']),
      ),
      'class' => array('draggable'),
    );
  }

  // Add table drag for weight field.
  drupal_add_tabledrag('detail-page-order', 'order', 'sibling', 'detail-page-order-weight');

  // Theme table.
  $output = theme('table', array(
    'header' => array(
      t('Field'),
      t('Enabled'),
      t('Label'),
      array('data' => t('Title'), 'class' => 'checkbox'),
      array('data' => t('Wide display'), 'class' => 'checkbox'),
      array('data' => t('Hide if empty'), 'class' => 'checkbox'),
      array('data' => t('0 is empty'), 'class' => 'checkbox'),
      t('Weight'),
    ),
    'rows' => $rows,
    'attributes' => array('id' => 'detail-page-order'),
    'empty' => t('No fields are configured to appear on the detail page. <a href="!url">Select some fields</a>.', array(
      '!url' => url("node/{$form['#node']->nid}/datavizwiz/manage"),
    )),
  ));

  // Render remaining form items.
  return $output . drupal_render_children($form);
}

/**
 * Detail page form - submit callback.
 */
function datavizwiz_detail_page_form_submit($form, $form_state) {
  // Flag the selected title field, and unflag the rest.
  foreach ($form_state['values']['fields'] as $field_name => $settings) {
    $form_state['values']['fields'][$field_name]['title'] = $field_name == $form_state['values']['title'];
  }

  // Update field settings.
  datavizwiz_merge_settings($form['#node'], $form_state['values']['fields'], array(
    'detail_back' => $form_state['values']['detail_back'],
  ));
  drupal_set_message(t('Detail page field settings updated.'));
}

/******************************************************************************
  Utilities
******************************************************************************/

/**
 * Merges DataVizWiz settings.
 */
function datavizwiz_merge_settings($node, $fields, $settings = array()) {
  // Merge field settings.
  foreach ($fields as $field_name => $field_settings) {
    if (!isset($node->datavizwiz['fields'][$field_name])) {
      $node->datavizwiz['fields'][$field_name] = array();
    }
    $node->datavizwiz['fields'][$field_name] = array_merge($node->datavizwiz['fields'][$field_name], $field_settings);
  }

  // Merge additional settings.
  $node->datavizwiz['settings'] = array_merge($node->datavizwiz['settings'], $settings);

  // Update field settings.
  db_update('datavizwiz')
    ->fields(array(
      'fields' => serialize($node->datavizwiz['fields']),
      'settings' => serialize($node->datavizwiz['settings']),
    ))
    ->condition('nid', $node->nid)
    ->execute();
}
