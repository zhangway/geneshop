<?php
/**
 * Theme API functions file.
 */
/**
 * Preprocess for view basket in block mode.
 */
function theme_geneshop_generate_table($vars) {
    $basket_items = isset($_SESSION['basket']['items']) ? $_SESSION['basket']['items'] : array();
    $items = array();
    $header = array();
    if (!empty($basket_items)) {
        foreach ($basket_items as $nid => $item) {
            $items[$nid] = l($item['content'], 'node/' . $nid);
            $header[0] = $item['header'];
        }
    }
    $cols = array();
    foreach($items as $nid => $content) {
        $cols[$nid] = explode("~", $content);
    }
    $headers = (isset($header[0]))? explode("~",$header[0]) : [];
    return array($basket_items, $cols, $headers);
}
function template_preprocess_geneshop_block_display(&$vars) {
  //error_log("TEMPLATE_PREPROCESS+++++++from".var_export(drupal_get_destination(), true));
  drupal_add_css(drupal_get_path('module', 'geneshop') . '/theme/geneshop.css');
  $referrer = drupal_get_destination();
  preg_match('#search/gene#' , $referrer['destination'], $matches1);
  if ( count($matches1) ) {	
    $basket_items = (isset($_SESSION['basket']['items']) && count($_SESSION['basket']['items']) > 0) ? 
                    $_SESSION['basket']['items'] : array();
  //error_log("items:".var_export($basket_items, true));
  } else {
    ;
  }
  preg_match('/ajax/', current_path(), $matches);
  if(!count($matches)){
    $_SESSION['view_page'] = current_path();
  }
  $variables = array(
      'path' => 'images/shopping-cart.png', 
      'alt' => 'Items in cart',
      'title' => 'Your shopping cart',
      'width' => '15',
      'height' => '15',
      'attributes' => array('class' => 'basket-img', 'id' => 'basketicon'),
      );
  $img = theme('image', $variables);

  list($basket_items, $cols, $headers) = theme_geneshop_generate_table($vars);
  $vars['not_empty'] = !empty($basket_items);
  $vars['count_items'] = $basket_items ? count($_SESSION['basket']['items']) : 0;
  $items_list = array(
    '#theme' => 'fieldset',
    '#title' => '<div class="basket-icon">'.$vars['count_items'].'&nbsp;items&nbsp;</div>' ,
    '#attached' => array(
      'js' => array(
        'misc/form.js',
        'misc/collapse.js',
      ),
    ),
    '#attributes' => array(
      'class' => array('collapsible', 'collapsed'),
    ),
    // change the type of display here:
    //'#value' => theme('item_list', array('items' => $item)),
    '#value' => theme('table', array('rows' => $cols, 'header' => $headers)),
  );
  //rendering multiple baskets:
  //$vars['basket_items_list'] = $basket_items ? drupal_render($items_list) . drupal_render($items_list2) : '';
  $vars['basket_items_list'] = $basket_items ? drupal_render($items_list) : '';
  $vars['total_sum'] = isset($_SESSION['basket']['total_sum']) ? _geneshop_get_price_numberformat($_SESSION['basket']['total_sum']) : FALSE;
  $vars['basket_page_link'] = $basket_items ? l(t('Go to basket page'), 'basket') : '';
}
function theme_geneshop_items_form($vars) {
  list($basket_items, $cols, $header) = theme_geneshop_generate_table($vars);
  //error_log('header:'.var_export($header, true));
  $header['del'] = t('Remove');
  $element = $vars['element'];
  foreach($cols as $nid => $c) {
      array_push($cols[$nid], "<input id='edit-items-".$nid."-del' class='form-checkbox' type='checkbox' value='1' name='items[".$nid."][del]'>");
  }
  $rows = array();
  foreach (element_children($element) as $key) {
      $row = array();
      $row['data'] = array();
      foreach ($header as $field => $title) {
          
          $row['data'][] = drupal_render($element[$key][$field]);
      }
      $rows[] = $row;
  }
  return theme('table', array(
    'header' => $header,
    'rows' => $cols,
  ));
}
function theme_geneshop_items_form_o($vars) {
  $header = _geneshop_basket_items_header();
  $header['del'] = t('Remove');
  $element = $vars['element'];
  $rows = array();
  foreach (element_children($element) as $key) {
    $row = array();
    $row['data'] = array();
    foreach ($header as $field => $title) {
      $row['data'][] = drupal_render($element[$key][$field]);
    }
    $rows[] = $row;
  }
  if (isset($_SESSION['basket']['total_sum'])) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => t('Order total sum'),
          'colspan' => 2,
        ),
        array(
          'data' => _geneshop_get_price_numberformat($_SESSION['basket']['total_sum']),
          'colspan' => 2,
        ),
      ),
      'id' => array('total-sum-tr'),
    );
  }
  return theme('table', array(
      'header' => $header,
      'rows' => $rows,
    ));
}
function theme_geneshop_items_table($vars) {
  
  $header = _geneshop_basket_items_header();
  $rows = $vars['basket']['items'];
  if (isset($vars['basket']['total_sum'])) {
    $currency = isset($vars['basket']['currency']) ? $vars['basket']['currency'] : '';
    foreach ($rows as $nid => $row) {
      $rows[$nid]['price'] = _geneshop_get_price_numberformat($row['price'], $currency);
    }
    $rows[] = array(
      'data' => array(
        array(
          'data' => t('Order total sum'),
          'colspan' => count($header) - 1,
        ),
        _geneshop_get_price_numberformat($vars['basket']['total_sum'], $currency),
      ),
    );
  }
  $items_table = array(
    '#theme' => 'fieldset',
    '#title' => t('Order items'),
    '#attached' => array(
      'js' => array(
        'misc/form.js',
        'misc/collapse.js',
      ),
    ),
    '#attributes' => array(
      'class' => array('collapsible', 'collapsed'),
    ),
    '#value' => theme('table', array('header' => $header, 'rows' => $rows)),
  );
  return drupal_render($items_table); 
}
function theme_geneshop_mail_notice($vars) {
  $output = array();
  if ($vars['destination'] == 'site') {
    $output[] = t('New order from the site "@site"', array('@site' => variable_get('site_name')));
    $output[] = t('On site "@site" has been added a new order.', array('@site' => variable_get('site_name')));
  }
  else {
    $output[] = t('You or someone entered your email address in a feedback, the order was made on the site "@site".', array('@site' => variable_get('site_name')));
  }
  $output[] = t('Link to the order page: @link', array('@link' => $vars['view_order_link']));
  $output[] = "==========================================================";
  $output[] = t("Order items:\n\n@order_items", array('@order_items' => $vars['order_items']));
  if ($vars['total_sum']) {
    $output[] = "----------------------------------------------------------";
    $output[] = t("Order total sum: @total_sum", array('@total_sum' => $vars['total_sum']));
  }
  $output[] = "==========================================================";
  if (isset($vars['fields_data'])) {
    $output[] = $vars['fields_data'];
  }
  return $output;
}
/**
 * Preprocess for mail notice template.
 */
function template_preprocess_geneshop_mail_notice(&$vars, $hook) {
  $node = $vars['node'];
  $vars['view_order_link'] = url('order/' . $node->nid, array('query' => array('c' => $node->order_code), 'absolute' => TRUE));
  $vars['order_mail'] = $node->order_mail;
  $vars['order_items'] = '';
  foreach ($node->order_items['items'] as $item) {
    $output = $item['title'];
    if (isset($item['qty'])) {
      $output .= ' | ' . t('qty: @qty', array('@qty' => $item['qty']));
    }
    if (isset($item['price'])) {
      $output .= ' | ' . t('price: @price ', array('@price' => _geneshop_get_price_numberformat($item['price'])));
    }
    $vars['order_items'] .= $output . "\n";
  }
  $vars['total_sum'] = isset($node->order_items['total_sum']) ? _geneshop_get_price_numberformat($node->order_items['total_sum']) : '';
  if ($fields = variable_get('geneshop_mail_fields_' . $vars['destination'], array())) {
    $instances = field_info_instances('node', geneshop_ORDER_NODETYPE);
    $fields_data = '';
    foreach ($fields as $field) {
      if ($field_array = $node->{$field}[LANGUAGE_NONE]) {
        $fields_data .= $instances[$field]['label'] . ': ';
        $fields_data .= isset($field_array[1]) ? "\n" : '';
        for ($i = 0; $i < count($field_array); $i++) {
          if (isset($field_array[$i]['value'])) {
            $fields_data .= $field_array[$i]['value'] . "\n";
          }
        }
        $fields_data .= "\n";
      }
    }
    $vars['fields_data'] = $fields_data;
  }
}
