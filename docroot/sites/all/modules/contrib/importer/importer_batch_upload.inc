<?php
/**
 * @file The part of the module that handles the upload of files.
 */


/**
 * Form for uploading zip files
 *
 * @global $db_type
 * @param  $form_state
 */
function importer_batch_file_uploade_form($form, &$form_state) {
  global $databases;
  $db_type = Database::getConnection()->databaseType();
  $file_dir = file_build_uri("");
  $dirs = file_scan_directory($file_dir, '/(.)*_importer_files/', array('recurse' => FALSE));
  $dirnames = array();
  foreach ($dirs as $dir) {
    $path = $file_dir . '/' . $dir->filename;
    if (file_prepare_directory($path)) {
      $dirnames[$dir->filename] = $dir->filename;
    }
  }
  $form['#attributes'] = array(
    'enctype' => "multipart/form-data"
  );
  $form['files'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload a .zip file'),
    '#collapsible' => FALSE
  );
  if (function_exists('zip_open')) {
    $render_tables = TRUE;
    if ($db_type == 'mysql' || $db_type == 'mysqli') {
      $sql = "SHOW TABLES LIKE 'import\_%'";
    }
    elseif ($db_type == 'pgsql') {
      $sql = "SELECT tablename FROM {pg_tables} WHERE tablename LIKE 'import_%' ORDER BY tablename";
    }
    else {
      drupal_set_message(t('Unrecognized database type %dbtype', array('%dbtype' => $db_type)));
      $render_tables = FALSE;
    }
    $result = db_query($sql);
    $options = array(
      '--'
    );
    if ($db_type == 'pgsql') {
      foreach ($result as $row) {
        $unprefixed = schema_unprefix_table($row->tablename);
        $options[$unprefixed] = $unprefixed;
      }
    }
    else {
      $colname = 'tables_in_' . $databases['default']['default']['database'] . ' (import\_%)';
      foreach ($result as $row) {
        $unprefixed = schema_unprefix_table($row->{$colname});
        $options[$unprefixed] = $unprefixed;
      }
    }
    $max_filesize = format_size(parse_size(file_upload_max_size()));

    if ($render_tables) {
      $form['files']['table'] = array(
        '#type' => 'fieldset',
        '#title' => t('Choose a table and a field to modify'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
      );

      $form['files']['table']['existing_tables'] = array(
        '#title' => 'Existing database tables',
        '#type' => 'select',
        '#options' => $options,
        '#multiple' => FALSE,
        '#description' => t('Tables which are created via importer module.'),
        '#ajax' => array(
          'event' => 'change',
          'method' => 'replace',
          'callback' => 'importer_js',
          'effect' => 'fade',
          'wrapper' => 'edit-table-fields-wrapper',
        ),
      );

      $form['files']['table']['table_fields_wrapper']['table_fields'] = array(
        '#title' => 'Table fields',
        '#type' => 'select',
        '#options' => array(
          '--' => '--'
        ),
        '#prefix' => '<div id="edit-table-fields-wrapper">',
        '#suffix' => '</div>',
        '#disabled' => !isset($form_state['values']['existing_tables']) || $form_state['values']['existing_tables'] == '0',
        '#multiple' => FALSE,
        '#description' => t('Choose a field of selected table that contains the file name.<br />The content of the field must be the same as filenames in the zip file.'),


      );
    }

    $form['files']['file_upload'] = array(
      '#type' => 'file',
      '#description' => t('Maximum Filesize: @max_filesize<br />Must be a ZIP file', array('@max_filesize' => $max_filesize)),
      '#attributes' => array(
        'accept' => 'zip'
      ),
      '#title' => 'Zip file to upload'
    );

    $form['files']['directory'] = array(
      '#type' => 'textfield',
      '#size' => 50,
      '#maxlength' => 255,
      '#title' => 'Directory to store files in',
      '#description' => t('Creates a new directory in the \'files\' directory. \'_importer_files\' will be appended to directory name.'),


    );

    if (count($dirnames) != 0) {
      $form['files']['existing_dirs'] = array(
        '#title' => 'Existing directories',
        '#type' => 'radios',
        '#required' => FALSE,
        '#options' => $dirnames,
        '#description' => t('Only valid if the new directory field is empty, in witch case you must choose one of the existing directories to store files in.'),


      );
    }

    $form['files']['file_upload_button'] = array(
      '#type' => 'submit',
      '#value' => t('Upload')
    );

  }
  else {
    $form['files'] = array(
      '#value' => t('Your php instalation doesn\'t have zip manipulation capabilities. Prease install zlib php library to use this feature.'),


    );
  }
  return $form;
}

/**
 * Menu callback, referenced by the #ajax['path'] property
 */
function importer_js($form, $form_state) {

  $tablename = db_escape_table($_REQUEST['existing_tables']);
  $options = array();
  if (db_table_exists($tablename)) {
    $results = db_query("show columns from {$tablename}");
    foreach ($results as $result) {
      if ($result->key !== "PRI") {
        $options[$result->field] = $result->field;
      }
    }
  }
  else {
    $options['--'] = '--';
  }

  //This has to be done because of a validation error that is reported otherwise
  $cached_form = form_get_cache($_POST['form_build_id'], $form_state);
  $cached_form['files']['table']['table_fields_wrapper']['table_fields']['#options'] = $options;
  form_set_cache($_POST['form_build_id'], $cached_form, $form_state);

  $form['files']['table']['table_fields_wrapper']['table_fields']['#options'] = $options;


  return $form['files']['table']['table_fields_wrapper']['table_fields'];
}

/**
 * Validation method for the upload zip form
 *
 * @param  $form
 * @param  $form_state
 */
function importer_batch_file_uploade_form_validate(& $form, & $form_state) {
  if (!isset ($form_state['values']['directory']) || trim($form_state['values']['directory']) == '') {
    if (!isset ($form_state['values']['existing_dirs']) || trim($form_state['values']['existing_dirs']) == '') {
      form_set_error('directory', t('If none of the existing directories is chosen or no appropriate directory exists, one must be created.'));
    }
  }
}

/**
 * Submit the _batch_file_uploade form
 * @param $form
 * @param $form_state
 */
function importer_batch_file_uploade_form_submit($form, & $form_state) {
  $dir_name = ((isset($form_state['values']['directory']) && trim($form_state['values']['directory']) != '') ? $form_state['values']['directory'] . '_importer_files' : $form_state['values']['existing_dirs']);
  $full_dir_path = file_build_uri($dir_name);

  if (!file_prepare_directory($full_dir_path, FILE_CREATE_DIRECTORY))
    watchdog('php', 'Directory %dir was NOT created.', array('%dir' => $dir_name), WATCHDOG_ERROR);

  $images = array();
  $validators = array(
    'file_validate_extensions' => array(
      'zip'
    ),
    'importer_valid_zip' => array(),


  );
  if ($file = file_save_upload('file_upload', $validators)) {
    $extracted = importer_zip_unpaker($file->uri, $full_dir_path, $form_state);
    if ($file->fid) {
      file_delete($file);
    }
  }
  drupal_set_message(t('Upload successful!'));
}

/**
 * Unpack the zip file, save the content, update the {files} table
 * and the import_* table, if one was selected on the form.
 *
 * @global $user
 * @param  $filepath. A peth to the zip file
 * @param  $save_filepath. Full path to the destination.
 * @param  $form_state
 */
function importer_zip_unpaker_new($filepath, $save_filepath, $form_state) {
  $extracted = array();
  global $user;

  if ($z = zip_open($filepath)) {
    $dest = file_destination(file_directory_temp() . '/' . basename($filepath, '.zip'), FILE_EXISTS_REPLACE);
  }
}

function importer_zip_unpaker($filepath, $save_filepath, $form_state) {
  $extracted = array();
  global $user;

  if ($z = zip_open(drupal_realpath($filepath))) {
    $dest = file_destination(file_directory_temp() . '/' . basename($filepath, '.zip'), FILE_EXISTS_REPLACE);
    file_prepare_directory($dest, FILE_CREATE_DIRECTORY);
    $msg_database = "";
    //File ids of the old files, if any
    $table_fids = array();
    $query = db_select('importer_change_log')
            ->condition('table_name', $form_state['values']['existing_tables'], '=')
            ->fields('importer_change_log');
    $results = $query->execute();
    $result = $results->fetchAssoc();
    $modified = FALSE;
    if (!empty($result)) {
      $modified = TRUE;
      $table_fids = unserialize($result['fids']);
    }
    $new_table_fields = array();
    while ($entry = zip_read($z)) {

      if (zip_entry_open($z, $entry, 'r') && ($zip_entry_filesize = zip_entry_filesize($entry))) {
        $entry_name = zip_entry_name($entry);
        $data = zip_entry_read($entry, $zip_entry_filesize);


        if ($modified) {
          if ($file = file_save_data($data, $save_filepath . '/' . drupal_strtolower($entry_name), FILE_EXISTS_REPLACE)) {

            if ($form_state['values']['existing_tables'] !== '--' && $form_state['values']['table_fields'] !== '--') {
              $old_fid = "";
              foreach ($table_fids as $fid) {
                $old_file = file_load($fid);
                if ($old_file->filename == $entry_name) {
                  $old_fid = $old_file->fid;
                  break;
                }
              }
              $query = db_select($form_state['values']['existing_tables'])
                      ->condition($form_state['values']['table_fields'], $old_fid, '=')
                      ->fields($form_state['values']['existing_tables']);
              $results = $query->execute();
              $result = $results->fetchAssoc();
              if ($id = $result['importer_id']) {
                $query = db_update($form_state['values']['existing_tables'])
                        ->condition('importer_id', $id, '=')
                        ->fields(array(db_escape_table($form_state['values']['table_fields']) => $file->fid));
                $query->execute();
                $new_table_fields[$file->fid] = $file->fid;
              }
            }
          }
        }
        else {
          if ($file = file_save_data($data, $save_filepath . '/' . drupal_strtolower($entry_name), FILE_EXISTS_REPLACE)) {

            if ($form_state['values']['existing_tables'] !== '--' && $form_state['values']['table_fields'] !== '--') {

              $query = db_update(db_escape_table($form_state['values']['existing_tables']))
                      ->fields(array(db_escape_table($form_state['values']['table_fields']) => $file->fid))
                      ->condition(db_escape_table($form_state['values']['table_fields']), $file->filename, '=');
              $query->execute();
              $new_table_fields[$file->fid] = $file->fid;
            }
          }
        }
      }

      $msg_file = t('File @file was succesfully saved in the directory @dir under the name %newfile',
                    array('@file' => $entry_name, '@dir' => $form_state['values']['directory'] ? $form_state['values']['directory'] : $form_state['values']['existing_dirs'], '%newfile' => $file->filename));
      drupal_set_message($msg_file);
      $extracted[] = $file;
      $msg_database = t('Table @table was sucessfully updated!', array('@table' => $form_state['values']['existing_tables']));
      zip_entry_close($entry);
    }

    $new_log_entry = new StdClass();
    $new_log_entry->table_name = $form_state['values']['existing_tables'];
    $new_log_entry->fids = serialize($new_table_fields);
    if ($modified) {
      drupal_write_record('importer_change_log', $new_log_entry, 'table_name');
    }
    else {
      drupal_write_record('importer_change_log', $new_log_entry);
    }
  }


  if ($msg_database != "") {
    drupal_set_message($msg_database);
  }

  zip_close($z);
  return $extracted;
}

/**
 * Is the uploaded file a valid zip file
 *
 * @param <type> $file
 */
function importer_valid_zip($file) {
  $errors = array();
  if ($z = zip_open($file->uri)) {
    zip_close($z);
  }
  else {
    $errors[] = t('The file is not a valid zip file or may be corrupt.');
  }
  return $errors;
}