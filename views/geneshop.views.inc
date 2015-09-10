<?php
/**
 * @file
 * Views hooks for Gene Shop.
 */
/**
 * Implements hook_views_data_alter().
 */
function geneshop_views_data_alter(&$data) {
  $data['node']['add_to_basket'] = array(
    'title' => t('Basket button'),
    'help' => t('The button "add to basket" for nodes that are defined as goods.'),
    'group' => t('Gene Shop'),
    'field' => array(
      'additional fields' => array(
        'nid' => array(
          'table' => 'node',
          'field' => 'nid',
        ),
        'type' => array(
          'table' => 'node',
          'field' => 'type',
        ),
      ),
      'handler' => 'geneshop_handler_field_node_add_to_basket',
    ),
  );
}
/**
 * Implements hook_views_data().
 */
function geneshop_views_data() {
  $data['geneshop_order']['table']['group'] = t('Gene Shop');
  $data['geneshop_order']['table']['join'] = array(
    'node' => array(
      'left_field' => 'nid',
      'field' => 'nid',
    ),
  );
  $data['geneshop_order']['oid'] = array(
    'title' => t('Order: ID'),
    'help' => t('The order ID.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['geneshop_order']['mail'] = array(
    'title' => t('Order: Mail'),
    'help' => t('E-mail that has been specified by the customer when ordering.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );
  $data['geneshop_order']['items'] = array(
    'title' => t('Order: Items'),
    'help' => t('The order items or the total amount.'),
    'field' => array(
      'handler' => 'geneshop_handler_field_geneshop_order_items',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );
  return $data;
}
