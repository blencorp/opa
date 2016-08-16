(function($) {
  Drupal.behaviors.datavizwizBarChart = {
    attach: function(context) {

      // Pane is initialized.
      $('.datavizwiz-pane-datavizwiz-bar-chart', context).bind('datavizwiz-initialize', function(event) {
        var $pane = $(this);
        $.each(Drupal.settings.datavizwiz.bar_chart, function(i, settings) {
          if (settings.id == $pane.attr('id')) {
            $pane.plot([{
              label: settings.label,
              data: settings.data,
              color: settings.color,
              bars: {
                show: true,
                barWidth: 0.3,
                align: 'center'
              },
              grid: {
                hoverable: true
              }
            }], {
              xaxis: {
                ticks: settings.ticks
              }
            });
          }
        });
      });

    }
  };
})(jQuery);