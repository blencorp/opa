(function($) {
  Drupal.behaviors.datavizwizContentTypeFieldsetSummary = {
    attach: function(context) {

      $('fieldset#edit-datavizwiz', context).drupalSetSummary(function(context) {
        if ($('#edit-datavizwiz-enabled', context).is(':checked')) {
          return Drupal.t('Enable data visualization');
        }
        else {
          return Drupal.t("Don't enable data visualization");
        }
      });

    }
  };
})(jQuery);
