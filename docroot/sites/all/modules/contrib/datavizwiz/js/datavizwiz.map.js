(function($) {
  Drupal.behaviors.datavizwizMap = {
    attach: function(context) {

      // Pane is initialized.
      $('.datavizwiz-pane-datavizwiz-map', context).bind('datavizwiz-initialize', function(event) {
        var $pane = $(this);
        var $clickbox = $pane.find('.datavizwiz-map-clickbox');
        var $hoverbox = $pane.find('.datavizwiz-map-hoverbox');

        $.each(Drupal.settings.datavizwiz.map, function(i, settings) {
          if (settings.id == $pane.attr('id')) {

            // Initialize layer.
            switch (settings.group_type) {
              case 'google':
                var map_layer = new OpenLayers.Layer.Google(null, {
                  sphericalMercator: true,
                  wrapDateLine: true,
                  transitionEffect: 'resize',
                  buffer: 1,
                  minZoomLevel: settings.zoom_minimum,
                  maxZoomLevel: settings.zoom_maximum
                });
                var zoom_initial = settings.zoom_initial - settings.zoom_minimum;
                break;

              case 'mapbox':
                var map_layer = new OpenLayers.Layer.XYZ(null, settings.layers, {
                  sphericalMercator: true,
                  wrapDateLine: true,
                  transitionEffect: 'resize',
                  buffer: 1,
                  serverResolutions: OpenLayers.Layer.Bing.prototype.serverResolutions,
                  resolutions: OpenLayers.Layer.Bing.prototype.serverResolutions.slice(settings.zoom_minimum, settings.zoom_maximum + 1)
                });
                var zoom_initial = settings.zoom_initial - settings.zoom_minimum;
                break;

              case 'open_street_maps':
                var map_layer = new OpenLayers.Layer.OSM(null, null, {
                  sphericalMercator: true,
                  wrapDateLine: true,
                  transitionEffect: 'resize',
                  buffer: 1,
                  serverResolutions: OpenLayers.Layer.Bing.prototype.serverResolutions,
                  resolutions: OpenLayers.Layer.Bing.prototype.serverResolutions.slice(settings.zoom_minimum, settings.zoom_maximum + 1)
                });
                var zoom_initial = settings.zoom_initial - settings.zoom_minimum;
                break;
            }

            // Render map.
            var map = new OpenLayers.Map({
              div: settings.id,
              layers: [map_layer],
              projection: 'EPSG:4326',
              controls: [
                new OpenLayers.Control.Navigation({
                  dragPanOptions: {
                    enableKinetic: true
                  }
                }),
                new OpenLayers.Control.Attribution(),
                new OpenLayers.Control.Zoom()
              ]
            });

            // Set initial map center and zoom level.
            map.setCenter(new OpenLayers.LonLat(settings.center[0], settings.center[1]).transform(
              new OpenLayers.Projection("EPSG:4326"), map.getProjectionObject()
            ), zoom_initial);

            // Get GeoJSON data.
            $.ajax({
              url: settings.geojson_url,
              type: 'POST',
              data: {
                title_field: settings.title_field,
                link_title: settings.link_title,
                popup_template: settings.popup
              },
              dataType: 'json',

              // Render map points.
              success: function(data) {
                baseProjection = map.projection;
                wgs84 = new OpenLayers.Projection('EPSG:4326');
                diff = new OpenLayers.Projection('EPSG:900913');
                var geojson_format = new OpenLayers.Format.GeoJSON({
                  'externalProjection': wgs84,
                  'internalProjection': diff
                });
  
                var style = new OpenLayers.Style({
                  pointRadius: '${radius}',
                  fillColor: settings.fill_color,
                  fillOpacity: 0.8,
                  strokeColor: settings.stroke_color,
                  strokeWidth: 2,
                  strokeOpacity: 0.8,
                  property: 'class'
                }, {
                  context: {
                    // Set the map point radius based on the number of
                    // clustered points, using a max radius so they don't get
                    // too large (unless set to 0). This is padded so the dots
                    // aren't too small.
                    radius: function(feature) {
                      return (settings.max_radius == 0 ? feature.attributes.count : Math.min(feature.attributes.count, settings.max_radius)) + 3;
                    }
                  }
                });

                var selectedStyle = new OpenLayers.Style({
                  fillColor: settings.select_fill_color,
                  strokeColor: settings.select_stroke_color
                });

                var vector_layer = new OpenLayers.Layer.Vector('Locations', {
                  projection: 'EPSG:4326',
                  strategies: [new OpenLayers.Strategy.Cluster()],
                  styleMap: new OpenLayers.StyleMap({
                    'default': style,
                    'select': selectedStyle
                  })
                });
                map.addLayer(vector_layer);
                var vector_data = geojson_format.read(data);
                vector_layer.addFeatures(vector_data);
        
                // Setup the popup. The OpenLayers popup dialog is used if a
                // popup template is provided; otherwise, the hover dialog is
                // used.
                var selectControl = new OpenLayers.Control.SelectFeature(vector_layer, {
                  hover: settings.popup.length == 0,
                  onSelect: function(feature) {
                    var selectedFeature = feature;
                    var list = [];
                    $.each(feature.cluster, function(i, row) {
                      if (row.attributes.fields[settings.title_field].length) {
                        list.push(row.attributes.popup);
                      }
                    });
                    var content = list.join('');
                    if (settings.popup.length > 0) {
                      var lonlat = feature.geometry.getBounds().getCenterLonLat();
                      feature.popup = new OpenLayers.Popup.FramedCloud('popup', lonlat, null, content, null, true, function() {
                        selectControl.unselect(selectedFeature);
                      });
                      feature.popup.panMapIfOutOfView = settings.popup_pan;
                      map.addPopup(feature.popup);
                    }
                    else {
                      $hoverbox.html(content).show();
                    }
                  },
                  onUnselect: function(feature) {
                    if (settings.popup.length > 0) {
                      map.removePopup(feature.popup);
                    }
                    $hoverbox.hide();
                  }
                });
                map.addControl(selectControl);
                selectControl.activate();

              }
            })
          }
        });
      });

    }
  };
})(jQuery);
