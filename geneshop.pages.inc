<?php

/**
 * Menu callbacks.
 */

/**
 * Submit actions for add2basket link.
 */
function geneshop_add2basket($node, $type) {
  // Verification of the absence of this node in basket.
  if (_geneshop_in_basket($node->nid)) {
    /*drupal_set_message(t('"@title" is already in your basket', array('@title' => $node->title)
      ), 'warning');*/
  }
  else {
    global $user;
    if ($user->uid == 0 && !drupal_session_started()) {
      // Must initialize sessions for anonymous users.
      drupal_session_start();
    }
    $item = array();
    $sql =<<<SQL
          SELECT g.name
          FROM  chado.gene g
          WHERE nid = :nid
SQL;
    $args = array(':nid' => $node->nid);
    $result = db_query($sql, $args)->fetchObject();
    $vars = get_object_vars ( $result );
    $item['title'] = $result->name;
    $item['content'] = $result->name;
    if (variable_get('geneshop_show_qty', TRUE)) {
      $item['qty'] = 1;
    }
    if (variable_get('geneshop_show_price', FALSE) && ($price = _geneshop_get_price($node))) {
      if ($price <= 0 || !is_numeric($price)
      ) {
        drupal_set_message(t('Sorry, but this product currently unavailable
        for ordering'), 'warning');
        $item = FALSE;
      }
      else {
        if (!isset($_SESSION['basket']['currency'])) {
          $_SESSION['basket']['currency'] = variable_get('geneshop_price_suffix', '');
        }
        if (!isset($_SESSION['basket']['total_sum'])) {
          $_SESSION['basket']['total_sum'] = 0;
        }
        $_SESSION['basket']['total_sum'] += $price;
        $item['price'] = $_SESSION['basket']['store'][$node->nid]['unit_price'] = $price;
      }
    }
    if ($item) {
      $_SESSION['basket']['items'][$node->nid] = $item;
      drupal_set_message(t("'@title' added in your basket", array('@title' => $node->title)));
    }
  }
  if ($type == 'ajax') {
    $basket_block = geneshop_block_content();
    $commands = array();
    $commands[] = ajax_command_replace(NULL, geneshop_add2basket_link($node->nid));
    $commands[] = ajax_command_replace('#geneshop-ajax-block-content', render($basket_block));
    $output = array('#type' => 'ajax', '#commands' => $commands);
    ajax_deliver($output);
  }
  else {
     // drupal_goto();
  }
}
function generate_fasta($sql, $regex) {
    $data = '';
    foreach ($_SESSION['basket']['items'] as $nid => $value) {
      $args = array(':nid' => $nid);
      $result = db_query($sql, $args)->fetchObject();
      if(strlen($result->name)>0) {
        $residues = preg_replace($regex, "$1\n", $result->residues);
        $data .= ">".$result->name."\n".$residues."\n";
      } 
    }
    //data needs to be print as a plain text:
    drupal_add_http_header('Content-Type: text/plain');
    print $data;
    drupal_exit();
}

/**
 * Shared Ajax callback for basket page actions.
 */
function geneshop_ajaxform_process() {
  if (!isset($_SESSION['basket']['items'])) {
    drupal_set_message(t('The form is invalid. Your basket is currently empty.'), 'error');
    return theme('status_messages');
  }
  $commands = array();
  $title = '';
  list($form, $form_state) = ajax_get_form();
  $element_name = $form_state['input']['_triggering_element_name'];
  foreach ($_SESSION['basket']['items'] as $nid => $data) {
    switch ($element_name) {
      case 'gene':
        $form = geneshop_gene();
        $form['#action'] = 'basket/gene';
        $title = t('Gene');
        break;
      case 'mrna':
        $form = geneshop_mrna();
        $form['#action'] = 'basket/mrna';
        $title = t('mRNA');
        break;
      case 'polypeptides':
        $form = geneshop_polypeptides();
        $form['#action'] = 'basket/polypeptides';
        $title = t('Polypeptides');
        break;
      default:
        drupal_process_form($form['#form_id'], $form, $form_state);
    }
  }
  switch ($element_name) {
    case 'clear':
      $form = drupal_get_form('_geneshop_clear_confirm');
      $form['#action'] = 'basket/clear';
      $title = t('Do you really want to delete all items from the basket?');
      break;
      
    case 'checkout':
      $form = geneshop_checkout(TRUE);
      $form['#action'] = 'basket/checkout';
      $title = t('Download');
      break;
      /*
    case 'polypeptides':
      $form = geneshop_polypeptides();
      $form['#action'] = 'basket/polypeptides';
      $title = t('Polypeptides');
      break;
    case 'mrna':      
      $form = geneshop_mrna();
      $form['#action'] = 'basket/mrna';
      $title = t('mRNA');
      break;
    case 'gene':
      $form = geneshop_gene();
      $form['#action'] = 'basket/gene';
      $title = t('Gene');
      break;*/
    default:
      drupal_process_form($form['#form_id'], $form, $form_state);
  }
  if ($element_name == 'new_order' && !form_get_errors()) {
    $form = array(
      'type' => 'item',
      'value' => node_view($form_state['node']),
    );
    $title = t('Your order has been successfully created');
  }
  $commands[] = ajax_command_replace(NULL, render($form));
  if ($title) {
    $commands[] = ajax_command_html('#geneshop-page-title *:first-child', $title);
  }
  return array('#type' => '', '#commands' => $commands);
}
/**
 * The form that displays the contents of basket and control buttons.
 */
function geneshop_basket_form($form, &$form_state) {
  // Build Breadcrumbs.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '');
  // Link to current URL.
  $breadcrumb[] = l("Basket", current_path());
  // Set Breadcrumbs.
  drupal_set_breadcrumb($breadcrumb);
  // Check for empty session.
  if (!isset($_SESSION['basket']['items'])) {
    drupal_goto($_SESSION['view_page']);
    drupal_set_title(t('Your basket is empty'));
    $form['empty'] = array(
      '#type' => 'markup',
      '#markup' => t('<p>There is nothing in your basket at the moment.</p>'),
    );
  }
  else {
    $form['#id'] = drupal_html_id('geneshop_basket_form');
    $form['#attributes'] = array(
      'class' => array('geneshop_basket'),
    );
    $ajax_settings = array(
      'path' => 'geneshop/ajax',
      'wrapper' => $form['#id'],
    );
    $form['clear'] = array(
      '#type' => 'submit',
      '#value' => t('Clear the basket'),
      '#name' => 'clear',
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('clear-button'),
      ),
    );
    $form['items'] = array(
      '#tree' => TRUE,
      '#theme' => 'geneshop_items_form',
    );
    $form['actions'] = array(
      '#type' => 'actions',
      '#attributes' => array(
        'class' => array('geneshop-form-actions'),
      ),
    );
    foreach ($_SESSION['basket']['items'] as $nid => $data) {
      $form['items'][$nid]['title'] = array(
        '#type' => 'link',
        '#title' => $data['title'],
        '#href' => 'node/' . $nid,
      );
      $form['items'][$nid]['actions'] = array(
        '#type' => 'actions',
        '#attributes' => array(
          'class' => array('geneshop-form-actions'),
        ),
      );
      $form['items'][$nid]['actions']['gene'] = array(
        '#type' => 'submit',
        '#name' => 'gene',
        '#value' => t('Download'),
        '#attributes' => array(
          'class' => array('checkout-button'),
        ),
      );
      $form['actions'][$nid]['actions']['mrna'] = array(
        '#type' => 'submit',
        '#name' => 'mrna',
        '#value' => t('Download'),
        '#attributes' => array(
          'class' => array('checkout-button'),
        ),
      );
      $form['actions'][$nid]['actions']['polypeptide'] = array(
        '#type' => 'submit',
        '#name' => 'polypeptides',
        '#value' => t('Download'),
        '#attributes' => array(
          'class' => array('checkout-button'),
        ),
      );
      if (isset($data['qty'])) {
        $form['items'][$nid]['qty'] = array(
          '#type' => 'textfield',
          '#default_value' => $data['qty'],
          '#size' => 4,
          '#element_validate' => array('_geneshop_qty_element_validate'),
          '#attributes' => array(
            'class' => array('qty'),
          ),
        );
      }
      if (isset($data['price'])) {
        $form['items'][$nid]['price'] = array(
          '#markup' => _geneshop_get_price_numberformat($data['price']),
          '#size' => 5,
        );
      }
      $form['items'][$nid]['del'] = array(
        '#type' => 'checkbox',
      );
    }
    $form['actions']['back'] = array (
        '#type' => 'submit',
        '#value' => t('Go Back'),
        '#name' => 'view',
        '#attributes' => array(
            'class' => array('back-button'),
            'redirect' => ($_SESSION['view_page']), //need to be in ()
        ),
    );
    
/*    $form['actions']['checkout'] = array(
      '#type' => 'submit',
      '#name' => 'checkout',
      '#value' => t('Download'),
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('checkout-button'),
      ),
    );
    
    // switch off ajax for :
    $form['actions']['polypeptides'] = array(
        '#type' => 'checkbox',
        '#submit' => array('geneshop_polypeptides'),
        '#value' => t('Polypeptides'),
        //'#ajax' => $ajax_settings,
        '#attributes' => array(
            'class' => array('checkout-button'),
        ),
    );
    $form['actions']['mrna'] = array(
        '#type' => 'checkbox',
        '#submit' => array('geneshop_mrna'),
        '#value' => t('mRNA'),
        '#ajax' => $ajax_settings,
        '#attributes' => array(
            'class' => array('checkout-button'),
        ),
    );
    $form['actions']['gene'] = array(
      '#type' => 'checkbox',
      '#submit' => array('geneshop_gene'),
      '#value' => t('Gene'),
      //'#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('checkout-button'),
      ),
    );  */
    $form['actions']['update'] = array(
      '#type' => 'submit',
      '#submit' => array('_geneshop_basket_update'),
      '#value' => t('Remove'),
      //'#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('update-button'),
      ),
    );
    $form['#validate'][] = 'geneshop_check_empty_basket';
    $form['#attached'] = array(
      'css' => array(
        drupal_get_path('module', 'geneshop') . '/theme/geneshop.css',
      ),
      'js' => array(
        drupal_get_path('module', 'geneshop') . '/theme/geneshop.js',
      ),
    );
  }
  return $form;
}
/**
 * Basket page form qty element validate callback.
 */
function _geneshop_qty_element_validate($element, &$form_state, $form) {
  if (!filter_var($element['#value'], FILTER_VALIDATE_INT) || $element['#value'] <= 0) {
    form_error($element, t('Incorrect value of qty'));
  }
}
/**
 * Basket page form submit callback.
 */
function geneshop_basket_form_submit($form, &$form_state) {
  switch ($form_state['triggering_element']['#name']) {
    case 'clear':
      $form_state['redirect'] = 'basket/clear';
      break;
    case 'checkout':
      $form_state['redirect'] = 'basket/checkout';
      break;
  }
  foreach ($_SESSION['basket']['items'] as $nid => $data) {
    switch ($form_state['triggering_element']['#name']) {
      case 'gene':
        $form_state['redirect'] = 'basket/gene';
        break;
      case 'mrna':
        $form_state['redirect'] = 'basket/mrna';
        break;
      case 'polypeptides':
        $form_state['redirect'] = 'basket/polypeptides';
        break;
    }
  }
      /*
    case 'polypeptides':
      $form_state['redirect'] = 'basket/polypeptides';
      break;
    case 'mrna':
      $form_state['redirect'] = 'basket/mrna';
      break;
    case 'gene':
      $form_state['redirect'] = 'basket/gene';
     break;
      */
}
/**
 * Handling data update form the basket page.
 */
function _geneshop_basket_update($form, &$form_state) {
  $values = $form_state['values']['items'];
  $updated = FALSE;
  foreach ($values as $key => $val) {
    if ($val['del']) {
      drupal_set_message(t('"%title" was removed from your basket', array(
            '%title' => $_SESSION['basket']['items'][$key]['title'],
          )));
      unset($_SESSION['basket']['items'][$key]);
      if (isset($_SESSION['basket']['store'][$key])) {
        unset($_SESSION['basket']['store'][$key]);
      }
      $updated = TRUE;
    }
    elseif (variable_get('geneshop_show_qty', TRUE) && $_SESSION['basket']['items'][$key]['qty'] != $val['qty']) {
      $_SESSION['basket']['items'][$key]['qty'] = $val['qty'];
      if (variable_get('geneshop_show_price', FALSE)) {
        $_SESSION['basket']['items'][$key]['price'] = $val['qty'] * $_SESSION['basket']['store'][$key]['unit_price'];
      }
      drupal_set_message(t('"%title": successfully updated', array(
            '%title' => $_SESSION['basket']['items'][$key]['title'],
          )));
      $updated = TRUE;
    }
  }
  if ($updated) {
    $form_state['rebuild'] = TRUE;
    _geneshop_update_total();
  }
}
/**
 * Updating total sum of basket items.
 */
function _geneshop_update_total() {
  if (empty($_SESSION['basket']['items'])) {
    unset($_SESSION['basket']);
  }
  elseif (variable_get('geneshop_show_price', FALSE) && variable_get('geneshop_price_field', '')) {
    $sum = 0;
    foreach ($_SESSION['basket']['items'] as $item) {
      $sum += $item['price'];
    }
    $_SESSION['basket']['total_sum'] = $sum;
  }
}
/**
 * Calling the checkout form.
*/
/*
function geneshop_checkout($ajax = FALSE) {
  module_load_include('inc', 'node', 'node.pages');
  $add_form = node_add('geneshop_order');
  return $ajax ? $add_form : drupal_render($add_form);
}
*/

function geneshop_polypeptides() {
        $sql =<<<SQL
            select f2.name, f2.residues
            from chado.gene g,
            chado.feature_relationship fr,
            chado.feature f,
            public.chado_feature chf,
            chado.cvterm cvt,
            chado.feature f2,
            chado.feature_relationship fr2,
                   chado.cvterm cvt2
              where g.gene_id = fr.object_id
                    and fr.subject_id = f.feature_id
                    and g.gene_id = chf.feature_id
                    and f.type_id = cvt.cvterm_id
                    and cvt.name =  'mRNA'
                    and f.feature_id = fr2.object_id
                    and fr2.subject_id = f2.feature_id
                    and f2.type_id = cvt2.cvterm_id
                    and cvt2.name ='polypeptide'
                    and chf.nid = :nid
SQL;
    $regex = "/([a-zA-Z]{80})/";
    generate_fasta($sql, $regex);
}

/*function geneshop_mrna($ajax = FALSE) {
    $sql =<<<SQL
            select f.name, f.residues
            from chado.gene g,
            chado.feature_relationship fr,
            chado.feature f,
                 public.chado_feature chf
            where g.gene_id = fr.object_id
                  and fr.subject_id = f.feature_id
                  and g.gene_id = chf.feature_id
            and chf.nid = :nid
SQL;
    $regex = "/([acgtACGT]{80})/";
    generate_fasta($sql, $regex);
}*/

/*function geneshop_gene() {
  $sql =<<<SQL
           select f.name, f.residues
           from chado.gene g,
           chado.feature f,
           chado.featureloc floc,
           public.chado_feature chf
           where g.gene_id = floc.feature_id
                 and floc.srcfeature_id = f.feature_id
                 and g.gene_id = chf.feature_id
                 and chf.nid = :nid
SQL;
  $regex = "/([acgtACGT]{50})/";
  generate_fasta($sql, $regex);
}*/

/*function generate_fasta_floc($sql, $regex) {
    $data = '';
    foreach ($_SESSION['basket']['items'] as $nid => $value) {
      $args = array(':nid' => $nid);
      $result = db_query($sql, $args)->fetchobject();
      $start = $result->fmin;
      $stop = $result->fmax;
      $residues = array();
      for ($i = 0; $i < ($stop-$start); $i++) {
        $j = $start;
        $residues[$i] = $results->residues[$j];
        $j++;
      }
     // $residue = preg_replace($regex, "$1\n", $residues);
      $data .= ">".$result->name;
    }
    //data needs to be print as a plain text:
    drupal_add_http_header('Content-Type: text/plain');
    print $data;
    drupal_exit();
}*/

/**
 * Confirmation form when cleaning basket.
 */
function _geneshop_clear_confirm($form, &$form_state) {
  if (isset($_SESSION['basket']['items'])) {
    return confirm_form($form,
      t('Do you really want to delete all items from the basket?'),
      'basket',
      t('This operation is irreversible'),
      t('Yes, clear'),
      t('Cancel')
    );
  }
}
/**
 * Confirmation form submit callback.
 */
function _geneshop_clear_confirm_submit($form, &$form_state) {
  unset($_SESSION['basket']);
  drupal_set_message(t('Your basket is cleared'));
  drupal_goto($_SESSION['view_page']);
}
/**
 * Helper function to check the access code when referring to a specific order.
 */
function _geneshop_get_my_order($node) {
  $code = isset($_GET['c']) && !empty($_GET['c']) ? check_plain($_GET['c']) : FALSE;
  $node_code = isset($node->order_code) ? $node->order_code : FALSE;
  if ($code && $node_code && ($code === $node_code)) {
    $_SESSION['order_codes'][] = $code;
    drupal_goto('node/' . $node->nid);
  }
  drupal_not_found();
}
/**
 * Menu callback; displays page containing recent orders of a given user.
 */
function geneshop_my_orders_page($account) {
  drupal_set_title(t("@name's Order History", array('@name' => format_username($account))), PASS_THROUGH);
  $build = array();
  $query = db_select('node', 'n')->extend('PagerDefault');
  $nids = $query->fields('n', array('nid', 'sticky', 'created'))->condition('type', geneshop_ORDER_NODETYPE)->condition('uid', $account->uid)->condition('status', 1)->orderBy('created', 'DESC')->limit(variable_get('default_nodes_main', 10))->addTag('node_access')->execute()->fetchCol();
  if (!empty($nids)) {
    $nodes = node_load_multiple($nids);
    $build += node_view_multiple($nodes);
    $build['pager'] = array(
      '#theme' => 'pager',
      '#weight' => 5,
    );
  }
  else {
    drupal_set_message(t('You have no orders for this account.'));
  }
  return $build;
}

/**
 * Use function tripal_get_feature_sequences() from tripal_feature
 * for getting the sequences of genes.
 */
function geneshop_gene() {
  module_load_include('inc', 'tripal_feature', 'tripal_feature.api');
  $sql =<<<SQL
    select g.gene_id, f.feature_id, floc.featureloc_id, g.name, cvt.name as type_name
    from chado.gene g,
         chado.feature f,
         chado.featureloc floc,
         chado.cvterm cvt,
         public.chado_feature chf
    where g.gene_id = floc.feature_id
      and floc.srcfeature_id = f.feature_id
      and f.type_id = cvt.cvterm_id
      and g.gene_id = chf.feature_id
      and chf.nid = :nid
SQL;
  foreach ($_SESSION['basket']['items'] as $nid => $value) {
    $args = array(':nid' => $nid);
    $result = db_query($sql, $args)->fetchObject();
    $num_bases = 50;
    $sequence = array();
    $sequence = tripal_get_feature_sequences(
      array(
        'feature_id' => $result->gene_id,
        'parent_id' => $result->feature_id,
        'featureloc_id'=> $result->featureloc_id,
      ),
      array(
        'width' => $num_bases, // FASTA sequence should have $num_bases chars per line
        'derive_from_parent' => 1,
        'aggregate' => 1,
        'is_html' => 0
      )
    );
    $data = '';
    if (count($sequence)>0) {
      $data .= ">".$result->name."\n";
      if ($sequence[0]['residues']) {
        $data .= $sequence[0][residues]."\n";
      }
    }
    drupal_add_http_header('Content-Type: text/plain');
    print $data;
    drupal_exit();
  }
}

function geneshop_mrna() {
  module_load_include('inc', 'tripal_feature', 'tripal_feature.api');
  $sql =<<<SQL
    select fs.feature_id, fo.feature_id as parent_id, floc.feature_id as featureloc_id,
           fs.name, cvt.name as relationship_type
    from chado.gene g,
         chado.feature fo,
         chado.feature_relationship fr,
         chado.feature fs,
         chado.featureloc floc,
         chado.cvterm cvt,
         public.chado_feature chf
    where g.gene_id = fo.feature_id
      and fo.feature_id = fr.object_id
      and fr.subject_id = fs.feature_id
      and fs.feature_id = floc.feature_id
      and fr.type_id = cvt.cvterm_id
      and g.gene_id = chf.feature_id
      and chf.nid = :nid
SQL;
  foreach ($_SESSION['basket']['items'] as $nid => $value) {
    $args = array(':nid' => $nid);
    $result = db_query($sql, $args)->fetchObject();
    $num_bases = 50;
    $sequence = array();
    $sequence = tripal_get_feature_sequences(
      array(
        'feature_id' => $result->feature_id,
        'parent_id' => $result->feature_id,
        'featureloc_id'=> $result->featureloc_id,
        'name' => $result->name,
      ),
      array(
        'width' => $num_bases, // FASTA sequence should have $num_bases chars per line
        'derive_from_parent' => 1,
        'aggregate' => 1,
        'relationship_type' => $result->relationship_type,
        'is_html' => 0
      )
    );
    $data = '';
    if (count($sequence)>0) {
      $data .= ">".$result->name."\n";
      if ($sequence[0]['residues']) {
        $data .= $sequence[0][residues]."\n";
      }
    }
    drupal_add_http_header('Content-Type: text/plain');
    print $data;
    drupal_exit();
  }
}