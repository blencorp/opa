(function($) {
  Drupal.behaviors.datavizwizPanes = {
    attach: function(context) {

      // Get menu tabs, menu links, and panes.
      var $tabs = $('.datavizwiz-panes-menu', context);
      var $links = $('a', $tabs);
      var $panes = $('.datavizwiz-pane', context);

      // A menu link is clicked.
      $links.click(function() {
        // Toggle link classes and visibility.
        $links.removeClass('active');
        $(this).addClass('active');

        // Toggle the corresponding pane.
        $panes.hide();
        var $pane = $panes.eq($links.index(this)).show();

        // Initialize the pane, and trigger event pane types can use.
        if (!$pane.data('datavizwiz-initialized')) {
          $pane.trigger('datavizwiz-initialize');
          $pane.data('datavizwiz-initialized', true);
        }
      });

      // If pane URL hash exists, trigger it.
      if (window.location.hash) {
        var pane_index = window.location.hash.replace(/#pane(\d+)/, '$1');
        $links.eq(pane_index).click();
      }

      // Toggle first menu link.
      else {
        $links.first().click();
      }

    }
  };
})(jQuery);
