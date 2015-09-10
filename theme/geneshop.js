(function($) {
  Drupal.behaviors.geneshop = {
    attach: function(context, settings) {
      var basketForm = $('form.geneshop_basket');
      var updBtn = $('input.update-button', basketForm);
      var backBtn = $('input.back-button', basketForm);
      var backBtn2 = $('input.back-button-2', basketForm);
      //updBtn.hide();
      //$('input', basketForm).change(function() {
      //    updBtn.show();
      //  });
      $('div.messages-ajax')
        .css("left", ($(window).width() - $('div.messages-ajax').outerWidth()) / 2 + $(window).scrollLeft() + "px")
        .delay(2000)
        .fadeOut(1000);

      backBtn.bind({
	  click: function() {          
            window.location.href = this.attributes.redirect.value;
            return false;  
	  }
      });
      backBtn2.bind({
          click: function() {
	          console.log("redirect: " + this.attributes.redirect.value);
            window.location.href = this.attributes.redirect.value;
            return false;
          }
      });
      var inputCountValue = $("input.qty", context);
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
