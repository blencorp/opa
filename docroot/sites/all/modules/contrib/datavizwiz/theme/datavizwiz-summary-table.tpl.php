<?php

/**
 * @file
 * Summary table.
 *
 * Available variables:
 * - $header: The table headers.
 * - $rows: The table row content.
 * - $pager_element: Used to correctly target pager.
 */
?>

<div class="datavizwiz-summary-table">
  <?php print theme('table', array('header' => $header, 'rows' => $rows)); ?>
</div>

<div class="datavizwiz-summary-pager">
  <?php print theme('pager', array('element' => $pager_element)); ?>
</div>