<?php

/**
 * @file
 * Panes.
 *
 * Available variables:
 * - $menu: An array of menu items.
 * - $panes: An array of pane data, each containing:
 *   - html_id: The unique HTML ID.
 *   - delta: The delta value in this array.
 *   - type: The pane type.
 *   - type_class: The pane type, with underscores converted to dashes.
 *   - render: The renderable content.
 *   - title: The raw pane title.
 *   - height: The raw pane height value.
 *   - height_style: Either something like "500px" or "auto".
 *   - data: An array of pane data.
 */
?>

<div class="datavizwiz-panes-menu">
  <?php print theme('item_list', array('items' => $menu, 'attributes' => array('class' => 'clearfix'))); ?>
</div>

<div class="datavizwiz-panes">
  <?php foreach ($panes as $pane): ?>
    <div id="<?php print $pane['html_id'] ?>" class="datavizwiz-pane datavizwiz-pane-<?php print $pane['delta']; ?> datavizwiz-pane-<?php print $pane['type_class']; ?>" style="height: <?php print $pane['height_style']; ?>">
      <?php print render($pane['render']); ?>
    </div>
  <?php endforeach; ?>
</div>
