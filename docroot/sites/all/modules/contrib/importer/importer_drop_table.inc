<?php
/**
 * @file Drop the elected table previously created via importer.
 */

/**
 * Handler for _drop_table form and corresponding confirmation
 *
 * @param  $form_state
 */
function _importer_drop_table($form, $form_state) {
  if (isset($form_state['values']['table_to_drop']) && $form_state['values']['op'] == 'Drop') {
    return _importer_drop_table_form_confirm($form_state);
  }
  if (isset($form_state['values']['filenames']) && count($form_state['values']['filenames']) != 0 && $form_state['values']['op'] == 'Delete') {
    return _importer_delete_file_form_confirm($form_state);
  }
  return _importer_drop_table_form($form_state);
}

/**
 * Form used to drop selected table
 *
 * @global $db_type
 * @param  $form_state
 */
function _importer_drop_table_form($form_state) {
  global $databases;
  $db_type = Database::getConnection()->databaseType();

  $sql = "";
  if ($db_type == 'mysql' || $db_type == 'mysqli') {
    $sql = "SHOW TABLES LIKE 'import\_%'";
  }
  elseif ($db_type == 'pgsql') {
    $sql = "SELECT tablename FROM {pg_tables} WHERE tablename LIKE 'import_%' ORDER BY tablename";
  }
  else {
    drupal_set_message(t('Unrecognized database type %dbtype', array(
                                                                    '%dbtype' => $db_type
                                                               )));
    $render_tables = FALSE;
  }
  $result = db_query($sql);
  $tables = array();
  $tables = array();
  if ($db_type == 'pgsql') {
    foreach ($result as $row) {
      $unprefixed = schema_unprefix_table($row->tablename);
      $tables[$unprefixed] = $unprefixed;
    }
  }
  else {
    $colname = 'tables_in_' . $databases['default']['default']['database'] . ' (import\_%)';
    foreach ($result as $row) {
      $unprefixed = schema_unprefix_table($row->{$colname});
      $tables[$unprefixed] = $unprefixed;
    }
  }
  if (!empty($tables)) {
    $form['drop_table_set'] = array(
      '#type' => 'fieldset',
      '#title' => t('Drop table'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE
    );

    $form['drop_table_set']['table_to_drop'] = array(
      '#type' => 'select',
      '#title' => 'Select table to drop',
      '#default_value' => $tables[key($tables)],
      '#options' => $tables,
    );


    $form['drop_table_set']['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Drop',
    );

  }
  $form['delete_file_set'] = array(
    '#type' => 'fieldset',
    '#title' => t('Delete files'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE
  );

  $files = importer_list_files(FALSE);

  $form['delete_file_set']['filenames'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => 'Select files to delete',
    '#options' => $files,
  );
  $form['delete_file_set']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Delete',
  );

  return $form;
}

/**
 * Confirmation for drop table action
 *
 * @param  $form_state
 */
function _importer_drop_table_form_confirm($form_state) {
  $desc = 'This action CAN NOT be undone.';
  // Tell the submit handler to process the form
  $form['process'] = array('#type' => 'hidden', '#value' => 'true');
  // Make sure the form redirects in the end
  $form['destination'] = array('#type' => 'hidden', '#value' => 'admin/content/importer/drop_table');
  $form['table_to_drop'] = array('#type' => 'hidden', '#value' => $form_state['values']['table_to_drop']);

  return confirm_form($form,
                      t('Are you sure you want to drop the table %table?', array('%table' => $form_state['values']['table_to_drop'])),
                      'admin/content/importer/drop_table',
                      $desc,
                      'Drop',
                      'Return');
}

/**
 * Confirmation for delete file action
 *
 * @param  $form_state
 */
function _importer_delete_file_form_confirm($form_state) {
  $desc = 'This action CAN NOT be undone.';
  // Tell the submit handler to process the form
  $form['process_files'] = array('#type' => 'hidden', '#value' => 'true');
  // Make sure the form redirects in the end
  $form['destination'] = array('#type' => 'hidden', '#value' => 'admin/content/importer/drop_table');
  $form['filenames'] = array('#type' => 'hidden', '#value' => implode(',', $form_state['values']['filenames']));

  return confirm_form($form,
                      t('Are you sure you want to delete selected file(s)?'),
                      'admin/content/importer/drop_table',
                      $desc,
                      'Delete',
                      'Return');
}

/**
 * Submit _drop_table form, or rebuild it if the
 * next step is a confirmation step
 *
 * @param  $form_id
 * @param  $form_state
 */
function _importer_drop_table_submit($form_id, &$form_state) {
  // If confirmation has been done, process the file
  if (isset($form_state['values']['process'])) {
    if (db_drop_table($form_state['values']['table_to_drop'])) {
      _importer_clean_log_table($form_state['values']['table_to_drop']);
      drupal_set_message('The table ' . $form_state['values']['table_to_drop'] . ' was successfully dropped.');
      if (module_exists('vidi')) {
        $query = db_delete('vidi_visualization_tables')
                ->condition('tablename', $form_state['values']['table_to_drop'], '=');
        if ($query->execute() === NULL) {
          drupal_set_message('The table ' . $form_state['values']['table_to_drop'] . ' failed to delete from VIDI handeling. Please do it manually.');
        }
      }
    }
    else
      drupal_set_message('The table ' . $form_state['values']['table_to_drop'] . ' was NOT dropped.');
    return TRUE;
  }
  elseif (isset($form_state['values']['process_files'])) {
    $filenames = explode(',', $form_state['values']['filenames']);
    $deleted_file = "";
    foreach ($filenames as $key => $value) {
      $path = file_build_uri('delimited/' . $value);
      $sql = "SELECT fid FROM {file_managed} WHERE uri = :uri";
      if ($result = db_query($sql, array(':uri' => $path))) {
        foreach ($result as $row) {
          $file = file_load($row->fid);
          $deleted_file = $file->filename;
          file_delete($file);
        }
      }
    }
    drupal_set_message('The file ' . $deleted_file . ' was successfully deleted.');
    return TRUE;
  }

  $form_state['rebuild'] = TRUE;
}

  /**
   * Clean the importer change log table of any records connected to
   * deleted table and delete files connected to it
   *
   * @param  $table_name
   */
function _importer_clean_log_table($table_name) {
  $query = db_select('importer_change_log')
          ->condition('table_name', $table_name, '=')
          ->fields('importer_change_log');
  $results = $query->execute();
  if ($result = $results->fetchAssoc()) {
    $fids = unserialize($result['fids']);
    foreach ($fids as $fid) {
      file_delete(file_load($fid));
    }
  }
  $query = db_delete('importer_change_log')
          ->condition('table_name', $table_name, '=');
  $query->execute();
}