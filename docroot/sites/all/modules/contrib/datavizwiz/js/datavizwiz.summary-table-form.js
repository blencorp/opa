(function($) {
  Drupal.behaviors.datavizwizSummaryTableForm = {
    attach: function(context) {

      // Make sure default sort field contains only sortable fields.
      $('input[id$=sortable]', context).click(function() {
        var $select = $('#edit-summary-sort-field', context);

        // Store value.
        var val = $select.val();

        // Remove all but first "- None -" option.
        $select.find('option').not(':first').remove();

        // Add only fields checked as sortable.
        $('input[id$=sortable]:checked', context).each(function() {
          $select.append($('<option/>').val($(this).data('datavizwiz-key')).text($(this).data('datavizwiz-key')));
        });

        // Restore value.
        $select.val(val);
      })

      // Trigger handler to setup initial options.
      .filter(':first').triggerHandler('click');

    }
  };
})(jQuery);
