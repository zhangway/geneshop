(function($) {
  Drupal.behaviors.geneshop_cart = {
    attach: function(context, settings) {
      var basketForm = $('form.geneshop_cart_basket');
      var updBtn = $('input.update-button', basketForm);
      updBtn.hide();
      $('input', basketForm).change(function() {
          updBtn.show();
        });
      $('div.messages-ajax')
        .css("left", ($(window).width() - $('div.messages-ajax').outerWidth()) / 2 + $(window).scrollLeft() + "px")
        .delay(2000)
        .fadeOut(1000); // Tongta delete Qty form
    }
  };
})(jQuery);
