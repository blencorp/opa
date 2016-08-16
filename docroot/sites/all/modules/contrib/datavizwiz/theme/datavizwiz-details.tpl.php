<?php

/**
 * @file
 * Details.
 *
 * Available variables:
 * - $meta: An array of meta node information.
 *   - url: A string containing node URL.
 *   - title: A string containing the field flagged as the page title.
 *   - back_link_text: The sanitized "back" link text.
 * - $details: An array of details, each containing:
 *   - classes: A string containing classes.
 *   - field_name: The detail field name.
 *   - label: The sanitized detail display label.
 *   - display_name: The sanitized detail display name.
 *   - value: The raw detail value.
 *   - value_safe: The sanitized detail value.
 */
?>

<?php if (!empty($meta['url']) && !empty($meta['back_link_text'])) { ?>
  <div class="datavizwiz-go-back">
    <a href="<?php print $meta['url']; ?>"><?php print $meta['back_link_text']; ?></a>
  </div>
<?php } ?>

<?php if (!empty($meta['title'])) { ?>
   <h1><?php print $meta['title']; ?></h1>
<?php } ?>

<div class="datavizwiz-details clearfix">
  <?php foreach ($details as $detail): ?>
    <div class="<?php print $detail['classes'] ?>">
      <div class="datavizwiz-detail-label"><?php print $detail['label']; ?></div>
      <div class="datavizwiz-detail-value"><?php print $detail['value_safe']; ?></div>
    </div>
  <?php endforeach; ?>
</div>
