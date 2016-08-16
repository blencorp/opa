<?php
/**
 * @file
 * opa_core.features.inc
 */

/**
 * Implements hook_node_info().
 */
function opa_core_node_info() {
  $items = array(
    'factsheet' => array(
      'name' => t('Factsheet'),
      'base' => 'node_content',
      'description' => t('A content type for factsheet.'),
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  drupal_alter('node_info', $items);
  return $items;
}
