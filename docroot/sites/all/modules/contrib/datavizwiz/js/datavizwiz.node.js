(function($) {
  Drupal.behaviors.datavizwizNodeFieldsetSummary = {
    attach: function(context) {

      $('fieldset#edit-datavizwiz', context).drupalSetSummary(function(context) {
        var data_table = $('#edit-datavizwiz-data-table', context).val();
        if (data_table) {
          return Drupal.checkPlain(data_table);
        }
        else {
          return Drupal.t('No table selected');
        }
      });

    }
  };
})(jQuery);
