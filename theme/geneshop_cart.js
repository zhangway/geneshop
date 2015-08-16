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
        .fadeOut(1000); // Tongta

      var inputCountValue = $("input.mRNA", context);
      var inputCountValue = $("input.CDs", context);
      inputCountValue.bind({
        keypress: function(e) {
          if (e.which != 13 && e.which != 8 && e.which !== 0 && (e.which < 48 || e.which > 57)) {
            return false;
          }
	},
        focus: function() {
          this.title = this.value;
          this.value = "";
        },
        blur: function() {
          if (this.value === "") {
            this.value = this.title;
          }
        }
      });
    }
  };
})(jQuery);
