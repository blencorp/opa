<?php

/******************************************************************************
  Data table
******************************************************************************/

/**
 * Data table form.
 */
function datavizwiz_data_table_form($form, $form_state, $node) {
  // Save node for submit callback.
  $form['#node'] = $node;

  // Filter form.
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter'),
    '#attributes' => array('class' => array('container-inline')),
    '#tree' => TRUE,
  );
  $form['filter']['field'] = array(
    '#type' => 'select',
    '#default_value' => !empty($_GET['field']) ? $_GET['field'] : NULL,
  );
  $form['filter']['operator'] = array(
    '#type' => 'select',
    '#options' => array(
      '=' => t('Is equal to'),
      '!=' => t('Is not equal to'),
      '<' => t('Is less than'),
      '<=' => t('Is less than or equal to'),
      '>' => t('Is greater than'),
      '>=' => t('Is greater than or equal to'),
      'LIKE' => t('Contains'),
      'NULL' => t('Is empty (NULL)'),
      'NOT NULL' => t('Is not empty (NOT NULL)'),
    ),
    '#default_value' => !empty($_GET['operator']) ? $_GET['operator'] : NULL,
  );
  $form['filter']['value'] = array(
    '#type' => 'textfield',
    '#default_value' => !empty($_GET['value']) ? $_GET['value'] : NULL,
  );
  $form['filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#submit' => array('datavizwiz_data_table_filter_submit'),
  );

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);

  // Build header from field name.
  $header['operations'] = t('Operations');
  foreach (array_keys($schemas[$node->datavizwiz['data_table']]['fields']) as $field_name) {
    // Skip disabled fields (unless none have been enabled).
    if (!empty($node->datavizwiz['data_fields']) && empty($node->datavizwiz['data_fields'][$field_name])) {
      continue;
    }

    $header[$field_name] = array(
      'data' => $field_name,
      'field' => "t.$field_name",
    );
  }

  // Build query.
  db_set_active($node->datavizwiz['connection']);
  $query = db_select($node->datavizwiz['data_table'], 't')->extend('PagerDefault')->extend('TableSort');

  // Add fields and filter condition.
  foreach (array_keys($schemas[$node->datavizwiz['data_table']]['fields']) as $field_name) {
    // Add field to filter options.
    $form['filter']['field']['#options'][$field_name] = $field_name;

    // Add filter condition.
    if (!empty($_GET['field']) && $_GET['field'] == $field_name && isset($_GET['value']) && !empty($_GET['operator'])) {
      // Wrap LIKE queries.
      if ($_GET['operator'] == 'LIKE') {
        $_GET['value'] = '%' . db_like($_GET['value']) . '%';
      }

      // NULL/NOT NULL handling is special.
      if (in_array($_GET['operator'], array('NULL', 'NOT NULL'))) {
        $query->{$_GET['operator'] == 'NULL' ? 'isNull' : 'isNotNull'}($_GET['field']);
      }
      // Everything else uses condition().
      else {
        $query->condition("t.$field_name", $_GET['value'], $_GET['operator']);
      }
    }

    // Skip disabled non-primary key fields (unless none have been enabled).
    if (!empty($node->datavizwiz['data_fields']) && empty($node->datavizwiz['fields'][$field_name]['data']) && !in_array($field_name, $node->datavizwiz['primary_keys'])) {
      continue;
    }

    // Add field to query.
    $query->addField('t', $field_name, "alias_$field_name");
  }

  // Finish query.
  $query->orderByHeader($header);
  $query->limit(30);
  $results = $query->execute()->fetchAll();
  db_set_active();

  // Remove field aliases.
  $rows = array();
  foreach ($results as $result) {
    // Remove field aliases.
    datavizwiz_remove_aliases($result);

    // Build tableselect option array with operation links.
    $rows[] = array_merge(array(
      'operations' => theme('links', array(
        'links' => array(
          array(
            'title' => t('view'),
            'href' => datavizwiz_detail_path($node, $result),
          ),
          array(
            'title' => t('edit'),
            'href' => datavizwiz_detail_path($node, $result, 'edit'),
            'query' => drupal_get_destination(),
          ),
          array(
            'title' => t('delete'),
            'href' => datavizwiz_detail_path($node, $result, 'delete'),
            'query' => drupal_get_destination(),
          ),
        ),
        'attributes' => array('class' => array('links', 'inline', 'nowrap')),
      )),
    ), (array) $result);
  }

  // Table select.
  $form['table'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('This table has no data.'),
  );

  // Pager.
  $form['pager'] = array('#markup' => theme('pager'));

  return $form;
}

/**
 * Data table form - filter submit callback.
 */
function datavizwiz_data_table_filter_submit($form, $form_state) {
  // Redirect to same page, but with the filter values as URL parameters.
  drupal_goto("node/{$form['#node']->nid}/datavizwiz/manage/data", array(
    'query' => array_diff_key($form_state['values']['filter'], array('submit' => NULL)),
  ));
}

/******************************************************************************
  Data row
******************************************************************************/

/**
 * Data row form.
 */
function datavizwiz_data_row_form($form, $form_state, $node) {
  // Get primary key values.
  $primary_key_values = array_slice(func_get_args(), 3);

  // Return 404 if we have wrong number of primary key values (unless creating,
  // in which case there will be no primary key values).
  if (!empty($primary_key_values) && count($primary_key_values) != count($node->datavizwiz['primary_keys'])) {
    drupal_not_found();
    exit;
  }

  // If editing, associate primary keys and values.
  $node->datavizwiz['primary_key_values'] = !empty($primary_key_values)
    ? array_combine($node->datavizwiz['primary_keys'], $primary_key_values)
    : array();

  // Save node in form for submit callback.
  $form['#node'] = $node;

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);

  // If editing, get row of field values where primary keys match.
  if (!empty($primary_key_values)) {
    db_set_active($node->datavizwiz['connection']);
    $query = db_select($node->datavizwiz['data_table'], 't');
    foreach (array_keys($schemas[$node->datavizwiz['data_table']]['fields']) as $field_name) {
      $query->addField('t', $field_name, "alias_$field_name");
    }
    foreach ($node->datavizwiz['primary_key_values'] as $field => $value) {
      $query->condition("t.$field", $value);
    }
    $result = $query->execute()->fetchObject();
    db_set_active();

    // Remove field aliases.
    datavizwiz_remove_aliases($result);
  }

  // Add form fields.
  $form['fields']['#tree'] = TRUE;
  foreach ($schemas[$node->datavizwiz['data_table']]['fields'] as $field_name => $schema) {
    // Determine field type.
    if ($schema['type'] == 'int' && $schema['size'] == 'tiny') {
      $type = 'checkbox';
    }
    else if ($schema['type'] == 'text') {
      $type = 'textarea';
    }
    else {
      $type = 'textfield';
    }

    // Form field, getting default value from edited row, then from schema.
    $form['fields'][$field_name] = array(
      '#type' => $type,
      '#title' => $field_name,
      '#default_value' => isset($result->$field_name)
        ? $result->$field_name
        : (isset($schema['default']) ? $schema['default'] : NULL),
      '#maxlength' => isset($schema['length']) ? $schema['length'] : NULL,
      '#disabled' => !empty($primary_key_values) && in_array($field_name, $node->datavizwiz['primary_keys']),
      '#description' => $schema['description'],
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
 * Data row form - submit callback.
 */
function datavizwiz_data_row_form_submit($form, $form_state) {
  $node = $form['#node'];

  // Get fields of visualization table.
  $schemas = schema_dbobject($node->datavizwiz['connection'])->inspect(NULL, $node->datavizwiz['data_table']);

  // Cast fields, and prevent serial fields from saving.
  foreach ($schemas[$node->datavizwiz['data_table']]['fields'] as $field_name => $schema) {
    if ($schema['type'] == 'serial') {
      unset($form_state['values']['fields'][$field_name]);
    }
    else if ($schema['type'] == 'int') {
      $form_state['values']['fields'][$field_name] = (int) $form_state['values']['fields'][$field_name];
    }
    else if ($schema['type'] == 'float') {
      $form_state['values']['fields'][$field_name] = (float) $form_state['values']['fields'][$field_name];
    }
  }

  // Write record.
  db_set_active($node->datavizwiz['connection']);
  db_merge($node->datavizwiz['data_table'])
    ->key(!empty($node->datavizwiz['primary_key_values'])
      ? $node->datavizwiz['primary_key_values']
      : array_fill_keys($node->datavizwiz['primary_keys'], NULL))
    ->fields(!empty($node->datavizwiz['primary_key_values'])
      ? array_diff_key($form_state['values']['fields'], $node->datavizwiz['primary_key_values'])
      : $form_state['values']['fields'])
    ->execute();
  db_set_active();

  drupal_set_message(t('The row has been saved.'));
  drupal_goto("node/$node->nid/datavizwiz/manage/data");
}

/**
 * Data row delete form.
 */
function datavizwiz_data_delete_form($form, $form_state, $node) {
  // Get primary key values.
  $primary_key_values = array_slice(func_get_args(), 3);

  // Return 404 if we have wrong number of primary key values.
  if (count($primary_key_values) != count($node->datavizwiz['primary_keys'])) {
    return MENU_NOT_FOUND;
  }

  $form['#node'] = $node;
  $form['primary_key_values'] = array(
    '#type' => 'value',
    '#value' => array_combine($node->datavizwiz['primary_keys'], $primary_key_values),
  );

  $question = t('Are you sure you want to delete this data row?');
  $description = '<p>' . t('This action cannot be undone.') . '</p>';
  return confirm_form($form, $question, "node/$node->nid/datavizwiz/manage/data", $description, t('Delete'));
}

/**
 * Data row delete form - submit callback.
 */
function datavizwiz_data_delete_form_submit($form, $form_state) {
  $node = $form['#node'];

  // Delete record.
  db_set_active($node->datavizwiz['connection']);
  $query = db_delete($node->datavizwiz['data_table']);
  foreach ($form_state['values']['primary_key_values'] as $field_name => $value) {
    $query->condition($field_name, $value);
  }
  $query->execute();
  db_set_active();

  drupal_set_message(t('The data row has been deleted.'));
  $form_state['redirect'] = "node/$node->nid/datavizwiz/manage/data";
}
