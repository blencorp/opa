<?php

/******************************************************************************
  Panes table form
******************************************************************************/

/**
 * Panes form.
 */
function datavizwiz_panes_form($form, $form_state, $node) {
  // Get panes for this node.
  $panes = db_query("SELECT * FROM {datavizwiz_panes} WHERE nid = :nid ORDER BY weight", array(
    ':nid' => $node->nid,
  ))->fetchAll();

  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Get pane types.
  $types = datavizwiz_pane_types();

  // Build form of panes.
  $form['panes']['#tree'] = TRUE;
  foreach ($panes as $pane) {
    $form['panes'][$pane->id]['title'] = array(
      '#markup' => check_plain($pane->title),
    );
    $form['panes'][$pane->id]['type'] = array(
      '#markup' => $types[$pane->type]['title'],
    );
    $form['panes'][$pane->id]['edit'] = array(
      '#markup' => l(t('edit'), "node/$node->nid/datavizwiz/manage/panes/$pane->id"),
    );
    $form['panes'][$pane->id]['delete'] = array(
      '#markup' => l(t('delete'), "node/$node->nid/datavizwiz/manage/panes/$pane->id/delete"),
    );
    $form['panes'][$pane->id]['weight'] = array(
      '#type' => 'weight',
      '#default_value' => $pane->weight,
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
 * Panes form - theme callback.
 */
function theme_datavizwiz_panes_form($variables) {
  // Get form.
  $form = $variables['form'];

  // Build table rows.
  $rows = array();
  foreach (element_children($form['panes']) as $id) {
    $form['panes'][$id]['weight']['#attributes']['class'][] = 'pane-order-weight';
    $rows[] = array(
      'data' => array(
        drupal_render($form['panes'][$id]['title']),
        drupal_render($form['panes'][$id]['type']),
        drupal_render($form['panes'][$id]['edit']),
        drupal_render($form['panes'][$id]['delete']),
        drupal_render($form['panes'][$id]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  // Add table drag for weight field.
  drupal_add_tabledrag('pane-order', 'order', 'sibling', 'pane-order-weight');

  // Theme table.
  $output = theme('table', array(
    'header' => array(
      t('Title'),
      t('Type'),
      array('data' => t('Operations'), 'colspan' => 2),
      t('Weight'),
    ),
    'rows' => $rows,
    'attributes' => array('id' => 'pane-order'),
    'empty' => t('There are visualization panes attached to this node.'),
  ));

  // Render remaining form items.
  return $output . drupal_render_children($form);
}

/**
 * Panes form - submit callback.
 */
function datavizwiz_panes_form_submit($form, $form_state) {
  // Update weights.
  foreach ($form_state['values']['panes'] as $id => $value) {
    db_update('datavizwiz_panes')
      ->fields(array('weight' => $value['weight']))
      ->condition('id', $id)
      ->execute();
  }
}

/******************************************************************************
  Pane form
******************************************************************************/

/**
 * Pane form.
 */
function datavizwiz_pane_form($form, $form_state, $node, $id = NULL) {
  // Save node for use in submit callback.
  $form['#node'] = $node;

  // Load pane.
  if ($id !== NULL) {
    $pane = db_query("SELECT * FROM {datavizwiz_panes} WHERE id = :id", array(
      ':id' => $id,
    ))->fetchObject();
    $pane->data = unserialize($pane->data);
    $form['id'] = array(
      '#type' => 'value',
      '#value' => $pane->id,
    );
  }

  // Title.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The pane tab text.'),
    '#default_value' => !empty($pane->title) ? $pane->title : '',
    '#required' => TRUE,
  );

  // Height.
  $form['height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#size' => 4,
    '#field_suffix' => t('px'),
    '#attributes' => array('style' => 'text-align: right'),
    '#description' => t('The height of the pane, in pixels. Enter "0" to use auto height.'),
    '#default_value' => !empty($pane->height) ? $pane->height : 500,
    '#required' => TRUE,
  );

  // Get pane types.
  $types = datavizwiz_pane_types();

  // Build options array.
  $options = array('' => '');
  foreach ($types as $key => $type) {
    $options[$key] = $type['title'];
  }

  // Type.
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#description' => t('The type of pane to render (additional fields may appear after selecting a value).'),
    '#options' => $options,
    '#default_value' => !empty($pane->type) ? $pane->type : '',
    '#ajax' => array(
      'callback' => 'datavizwiz_pane_data_js',
      'wrapper' => 'datavizwiz-pane-data',
    ),
    '#required' => TRUE,
  );

  // Pane data.
  $form['data'] = array(
    '#prefix' => '<div id="datavizwiz-pane-data">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );

  // Determine type.
  $type = !empty($form_state['values']['type'])
    ? $form_state['values']['type']
    : (!empty($pane->type) ? $pane->type : '');

  // Add more fields based on type.
  if (!empty($type) && !empty($types[$type]['form'])) {
    // Require file if necessary.
    if (!empty($types[$type]['file'])) {
      require_once drupal_get_path('module', $types[$type]['module']) . '/' . $types[$type]['file'];
    }
    $form['data'] = array_merge($form['data'], call_user_func($types[$type]['form'], $node, !empty($pane) ? $pane : NULL));
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
 * Pane form - submit callback.
 */
function datavizwiz_pane_form_submit($form, $form_state) {
  // Save pane.
  db_merge('datavizwiz_panes')
    ->key(array('id' => !empty($form_state['values']['id']) ? $form_state['values']['id'] : NULL))
    ->fields(array(
      'nid' => $form['#node']->nid,
      'title' => $form_state['values']['title'],
      'height' => $form_state['values']['height'],
      'type' => $form_state['values']['type'],
      'data' => serialize($form_state['values']['data']),
    ))
    ->execute();

  // Show message and redirect.
  drupal_set_message(t('The pane %title was saved.', array(
    '%title' => $form_state['values']['title'],
  )));
  drupal_goto("node/{$form['#node']->nid}/datavizwiz/manage/panes");
}

/******************************************************************************
  Pane delete form
******************************************************************************/

/**
 * Pane delete form.
 */
function datavizwiz_pane_delete_form($form, &$form_state, $node, $id) {
  // Load pane.
  $pane = db_query("SELECT * FROM {datavizwiz_panes} WHERE id = :id", array(
    ':id' => $id,
  ))->fetchObject();

  // Save node for use in submit callback.
  $form['#node'] = $node;

  $form['id'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  $message = t('Are you sure you want to delete the pane %title?', array(
    '%title' => $pane->title,
  ));
  $caption = '<p>' . t('This action cannot be undone.') . '</p>';
  return confirm_form($form, $message, "node/$node->nid/datavizwiz/manage/panes", $caption, t('Delete'));
}

/**
 * Pane delete form - submit callback.
 */
function datavizwiz_pane_delete_form_submit($form, $form_state) {
  // Delete, notify, and redirect.
  db_delete('datavizwiz_panes')->condition('id', $form_state['values']['id'])->execute();
  drupal_set_message(t('The pane was deleted.'));
  drupal_goto("node/{$form['#node']->nid}/datavizwiz/manage/panes");
}
