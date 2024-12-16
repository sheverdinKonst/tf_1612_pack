document.addEventListener("DOMContentLoaded", () => {

    $(document).on('click', '.menu__item', function(e) {

        const item = $(e.currentTarget);

        
        // const isActive = item.hasClass("open");
        
        // $(".menu__item").removeClass('open');
        
        // item.addClass("open");

        item.toggleClass("open");

    });

    $(document).on('click', '#js-ham', function(e) {
        $('body').toggleClass('menu-open')
    });

    $(document).on('click', '.page__content', function(e) {
        $('body').removeClass('menu-open')
    });

    

    

});