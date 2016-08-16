(function($) {
  Drupal.behaviors.datavizwizPieGraph = {
    attach: function(context) {

      // Pane is initialized; plot the pie graph.
      $('.datavizwiz-pane-datavizwiz-pie-graph', context).bind('datavizwiz-initialize', function(event) {
        var $pane = $(this);
        $.each(Drupal.settings.datavizwiz.pie_graph, function(i, settings) {
          if (settings.id == $pane.attr('id')) {
            $pane.plot(settings.data, {
              legend: {
                show: true,
                labelFormatter: function(label, series) {
                  var percent = Math.round(series.percent);
                  var count = series.data[0][1];
                  return('' + label + '</td><td class="legendCount">' + count + '</td><td class="legendPercent">' + percent + '%');
                }
              },
              series: {
                pie: {
                  show: true,
                  radius: 0.95,
                  label: {
                    show: true,
                    radius: 3/5,
                    formatter: function(label, series) {
                      // Strip stuff appended during legend label formatting.
                      label = label.replace(/([^<]+).*/, '$1');
                      return '<div class="datavizwiz-pie-label">' + label + '<br>' + Math.round(series.percent) + '%</div>';
                    },
                    threshold: 0.1
                  }
                }
              },
              grid: {
                clickable: true
              }
            });
          }
        });
      });

    }
  };
})(jQuery);