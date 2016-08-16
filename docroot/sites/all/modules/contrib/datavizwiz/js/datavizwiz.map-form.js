(function($) {
  Drupal.behaviors.datavizwizMapForm = {
    attach: function(context) {

      // A "Popup content" description link was clicked.
      $('.datavizwiz-map-popup', context).click(function() {
        // Get textarea and clicked token text.
        var textarea = $('#edit-data-popup', context).get(0);
        var token = $(this).text();

        // Borrowed from Insert module.
        if (document.selection) {
          textarea.focus();
          selection = document.selection.createRange();
          selection.text = token;
        }
        else if (textarea.selectionStart || textarea.selectionStart == '0') {
          var start = textarea.selectionStart;
          var end = textarea.selectionEnd;
          textarea.value = textarea.value.substring(0, start) + token + textarea.value.substring(end, textarea.value.length);
        }
        else {
          textarea.value += token;
        }

        return false;
      });

      // The map type field was changed.
      $('#edit-data-type', context).change(function() {
        // Change zoom select options.
        var settings = Drupal.settings.datavizwiz.map_types[$(this).val()];
        $('#edit-data-zoom-minimum, #edit-data-zoom-maximum, #edit-data-zoom-initial', context).each(function() {
          var val = $(this).val();
          $(this).empty();
          for (var i = settings.minzoom; i <= settings.maxzoom; i++) {
            $(this).append($('<option/>').val(i).text(i));
          }
          $(this).val(val);
        });

        // Toggle terms.
        $('span[id^=map-terms]', context).hide().filter('#map-terms-' + $(this).val()).show();
      }).change();

    }
  };
})(jQuery);
