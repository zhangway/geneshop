<?php

/**
 * @file
 * Page callbacks for module main settings.
 */
function geneshop_cart_admin_settings($form, &$form_state) {
  $form['#prefix'] = '<div id="geneshop_cart_settings_form-wrapper">';
  $form['#suffix'] = '</div>';
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );
  $form['general']['geneshop_cart_basket_email'] = array(
    '#type' => 'textfield',
    '#title' => t('E-mail'),
    '#required' => TRUE,
    '#default_value' => variable_get('geneshop_cart_basket_email', variable_get('site_mail', ini_get('sendmail_from'))),
    '#description' => t('E-mail address to which the order will be sent.'),
  );
  $form['general']['geneshop_cart_order_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Order title'),
    '#required' => TRUE,
    '#default_value' => variable_get('geneshop_cart_order_title', t('Order #[node:order-id] | [node:created]')),
    '#description' => t('This string will be used as title for orders. Use the syntax [token] if you want to insert a replacement pattern.'),
  );
  $form['general']['title_tokens'] = array(
    '#type' => 'fieldset',
    '#title' => t('Replacement patterns'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $info = token_info();
  $tokens = array();
  foreach ($info['tokens']['node'] as $name => $token) {
    if ($name == 'title') {
      continue;
    }
    $tokens[] = '[node:' . $name . '] - ' . $token['description'];
  }

  $form['general']['title_tokens']['list'] = array(
    '#markup' => implode('<br />', $tokens),
  );

  $options = node_type_get_names();

  unset($options[geneshop_cart_ORDER_NODETYPE]);
  $form['general']['geneshop_cart_node_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content types for Gene Shop'),
    '#options' => $options,
    '#default_value' => variable_get('geneshop_cart_node_types', array()),
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'geneshop_cart_settings_form_ajax_callback',
      'wrapper' => 'geneshop_cart_settings_form-wrapper',
    ),
  );


  $basket_types = isset($form_state['values']['geneshop_cart_node_types']) ? array_keys(array_filter($form_state['values']['geneshop_cart_node_types'])) : variable_get('geneshop_cart_node_types', array());

  if ($basket_types) {
    $form['display_options'] = array(
      '#type' => 'fieldset',
      '#title' => t('Display settings'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
    );  // Tongta
    $form['display_options']['geneshop_cart_show_mRNA'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display mRNA'),
      '#default_value' => variable_get('geneshop_cart_show_mRNA', TRUE),
    );
    $form['display_options']['geneshop_cart_show_CDs'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display CDs'),
      '#default_value' => variable_get('geneshop_cart_show_CDs', TRUE),
    );
  }

  $form['array_filter'] = array(
    '#type' => 'value',
    '#value' => TRUE,
  );
  $form['#submit'] = array('drupal_flush_all_caches');
  return system_settings_form($form);
}

function geneshop_cart_settings_form_ajax_callback($form, &$form_state) {
  return $form;
}

function geneshop_cart_admin_settings_validate($form, &$form_state) {
  $values = &$form_state['values'];
  $types = array_filter($values['geneshop_cart_node_types']);
  if (!$types) {
    $values['geneshop_cart_show_price'] = FALSE;
    $values['geneshop_cart_price_field'] = '';
    $values['geneshop_cart_price_suffix'] = '';
    return;
  }
  if (isset($values['geneshop_cart_show_price']) && $values['geneshop_cart_show_price'] == TRUE) {
    if (!empty($values['geneshop_cart_price_field'])) {
      foreach ($types as $type) {
        if (!field_info_instance('node', $values['geneshop_cart_price_field'], $type)) {
          form_set_error('geneshop_cart_price_field', t('The selected field %geneshop_cart_price_field is not enabled in the node type %type', array('%geneshop_cart_price_field' => $values['geneshop_cart_price_field'], '%type' => $type)));
        }
      }
    }
  }
  $mail_fields = variable_get('geneshop_cart_mail_fields', array());
  if (empty($mail_fields)) {
    $form_state['redirect'] = 'admin/config/geneshop_cart/mail';
  }
}

function geneshop_cart_admin_mail_settings($form, &$form_state) {
  $form['#prefix'] = t('Showing only those fields that can be displayed in plain text in emails.');
  $form['user'] = array(
    '#type' => 'fieldset',
    '#title' => t('Email notification settings for user'),
  );
  $form['user']['geneshop_cart_mail_user_send'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send order data on user mail.'),
    '#default_value' => variable_get('geneshop_cart_mail_user_send', FALSE),
  );
  if ($fields = field_info_instances('node', geneshop_cart_ORDER_NODETYPE)) {
    $fields_list = array();
    // Only those fields that can be displayed in plain text in emails.
    $field_modules_filter = array('number', 'text', 'list', 'options');
    foreach ($fields as $name => $field) {
      if (in_array($field['display']['default']['module'], $field_modules_filter)) {
        $fields_list[$name] = $field['label'] . ' (' . $name . ')';
      }
    }
    $form['site'] = array(
      '#type' => 'fieldset',
      '#title' => t('Email notification settings for site'),
    );
    $form['site']['geneshop_cart_mail_fields_site'] = array(
      '#type' => 'checkboxes',
      '#options' => $fields_list,
      '#title' => t('These fields, which will be sent to the email address of the site'),
      '#default_value' => variable_get('geneshop_cart_mail_fields_site', array()),
    );
    $form['user']['geneshop_cart_mail_fields_user'] = array(
      '#type' => 'checkboxes',
      '#options' => $fields_list,
      '#title' => t('These fields, which will be sent to the email address of the user'),
      '#default_value' => variable_get('geneshop_cart_mail_fields_user', array()),
      '#states' => array(
        'visible' => array(
          ':input[name="geneshop_cart_mail_user_send"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['array_filter'] = array(
      '#type' => 'value',
      '#value' => TRUE,
    );
  }

  return system_settings_form($form);
}
