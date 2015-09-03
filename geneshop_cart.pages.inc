<?php
/**
 * Menu callbacks.
 */
/**
 * Submit actions for add2basket link.
 */
function geneshop_cart_add2basket($node, $type) {
  // Verification of the absence of this node in basket.
  if (_geneshop_cart_in_basket($node->nid)) {
    drupal_set_message(t('"@title" is already in your basket', array('@title' => $node->title)
      ), 'warning');
  }
  else {
    global $user;
    if ($user->uid == 0 && !drupal_session_started()) {
      // Must initialize sessions for anonymous users.
      drupal_session_start();
    }
    $item = array();
    if ($node->type == 'chado_feature' && $node->feature->type_id->name == 'gene') {
      $sql =<<<SQL
            SELECT *
            FROM  chado.gene
            WHERE nid = :nid
SQL;
      $args = array(':nid' => $node->nid);
      $result = db_query($sql, $args)->fetchObject();
      $vars = get_object_vars ( $result );
      $item['type']  = "gene";
      $item['title'] = $result->name;
      $item['content'] = $result->name. "~" . $result->coordinate . "~" . $result->start . "~" . $result->stop . "~" . $result->gene_family ."~" . $result->description;
      $item['header'] = "Name~Chromosome~Start~End~Gene family~Description" ;    
    } else {
        $item['type']  = "item";
        $item['title'] = $node->title;
        $item['content'] = $node->title;
        $item['header'] = "Title";
    }
    if (variable_get('geneshop_cart_show_qty', TRUE)) {
      $item['qty'] = 1;
    }
    if (variable_get('geneshop_cart_show_price', FALSE) && ($price = _geneshop_cart_get_price($node))) {
      if ($price <= 0 || !is_numeric($price)
      ) {
        drupal_set_message(t('Sorry, but this product currently unavailable
        for ordering'), 'warning');
        $item = FALSE;
      }
      else {
        if (!isset($_SESSION['basket']['currency'])) {
          $_SESSION['basket']['currency'] = variable_get('geneshop_cart_price_suffix', '');
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
    $basket_block = geneshop_cart_block_content();
    $commands = array();
    $commands[] = ajax_command_replace(NULL, geneshop_cart_add2basket_link($node->nid));
    $commands[] = ajax_command_replace('#geneshop_cart-ajax-block-content', render($basket_block));
    $output = array('#type' => 'ajax', '#commands' => $commands);
    ajax_deliver($output);
  }
  else {
      //drupal_goto();
  }
}
function generate_fasta($sql, $regex) {
    $data = '';
    foreach ($_SESSION['basket']['items'] as $nid => $value) {
      $args = array(':nid' => $nid);
      $result = db_query($sql, $args)->fetchObject();
      if(strlen($result->residues)>0) {
        $residues = preg_replace($regex, "$1\n", $result->residues);
        $data .= ">".$result->name."\n".$residues."\n";
      } 
    }
    //data needs to be print as a plain text:
    drupal_add_http_header('Content-Type: text/plain');
    print $data;
    // drupal_exit prompts a save dialog:
    drupal_exit();
}

/**
 * Performs a reverse compliment of a nucleotide sequence
 *
 * @param $sequence
 *   The nucelotide sequence
 *
 * @return
 *   an upper-case reverse complemented sequence
 *
 * @ingroup tripal_feature_api
 */
/*function tripal_reverse_compliment_sequence($sequence) {

  $seq = strtoupper($sequence);
  $seq = strrev($seq);
  $seq = str_replace("A", "t", $seq);
  $seq = str_replace("T", "a", $seq);
  $seq = str_replace("G", "c", $seq);
  $seq = str_replace("C", "g", $seq);
  $seq = str_replace("Y", "r", $seq);
  $seq = str_replace("R", "y", $seq);
  $seq = str_replace("W", "w", $seq);
  $seq = str_replace("S", "s", $seq);
  $seq = str_replace("K", "m", $seq);
  $seq = str_replace("M", "k", $seq);
  $seq = str_replace("D", "h", $seq);
  $seq = str_replace("V", "b", $seq);
  $seq = str_replace("H", "d", $seq);
  $seq = str_replace("B", "v", $seq);
  return strtoupper($seq);
}*/

/**
 * Shared Ajax callback for basket page actions.
 */
function geneshop_cart_ajaxform_process() {
  if (!isset($_SESSION['basket']['items'])) {
    drupal_set_message(t('The form is invalid. Your basket is currently empty.'), 'error');
    return theme('status_messages');
  }
  $commands = array();
  $title = '';
  list($form, $form_state) = ajax_get_form();
  $element_name = $form_state['input']['_triggering_element_name'];
  switch ($element_name) {
    case 'clear':
      $form = drupal_get_form('_geneshop_cart_clear_confirm');
      $form['#action'] = 'basket/clear';
      $title = t('Do you really want to delete all items from the basket?');
      break;
      /*
    case 'checkout':
      $form = geneshop_cart_checkout(TRUE);
      $form['#action'] = 'basket/checkout';
      $title = t('Checkout');
      break;
      */
    case 'polypeptides':
      $form = geneshop_cart_polypeptides();
      $form['#action'] = 'basket/polypeptides';
      $title = t('Polypeptides');
      break;
    case 'mrna':      
      $form = geneshop_cart_mrna();
      $form['#action'] = 'basket/mrna';
      $title = t('mRNA');
      break;
    case 'gene':
      $form = geneshop_cart_gene();
      $form['#action'] = 'basket/gene';
      $title = t('Gene');
      break;
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
    $commands[] = ajax_command_html('#geneshop_cart-page-title *:first-child', $title);
  }
  return array('#type' => '', '#commands' => $commands);
}
/**
 * The form that displays the contents of basket and control buttons.
 */
function geneshop_cart_basket_form($form, &$form_state) {
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
    $form['#id'] = drupal_html_id('geneshop_cart_basket_form');
    $form['#attributes'] = array(
      'class' => array('geneshop_cart_basket'),
    );
    $ajax_settings = array(
      'path' => 'geneshop_cart/ajax',
      'wrapper' => $form['#id'],
    );
    $form['clear'] = array(
      '#type' => 'submit',
      '#value' => t('Clear the basket'),
      '#name' => 'clear',
      //'#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('clear-button'),
      ),
    );
    $form['items'] = array(
      '#tree' => TRUE,
      '#theme' => 'geneshop_cart_items_form',
    );
    foreach ($_SESSION['basket']['items'] as $nid => $data) {
      $form['items'][$nid]['title'] = array(
        '#type' => 'link',
        '#title' => $data['title'],
        '#href' => 'node/' . $nid,
      );
      if (isset($data['qty'])) {
        $form['items'][$nid]['qty'] = array(
          '#type' => 'textfield',
          '#default_value' => $data['qty'],
          '#size' => 4,
          '#element_validate' => array('_geneshop_cart_qty_element_validate'),
          '#attributes' => array(
            'class' => array('qty'),
          ),
        );
      }
      if (isset($data['price'])) {
        $form['items'][$nid]['price'] = array(
          '#markup' => _geneshop_cart_get_price_numberformat($data['price']),
          '#size' => 5,
        );
      }
      $form['items'][$nid]['del'] = array(
        '#type' => 'checkbox',
      );
    }
    $form['actions'] = array(
      '#type' => 'actions',
      '#attributes' => array(
        'class' => array('geneshop_cart-form-actions'),
      ),
    );
    $form['actions']['back'] = array (
        '#type' => 'submit',
        '#value' => t('Go Back'),
        '#name' => 'view',
        '#attributes' => array(
            'class' => array('back-button'),
            'redirect' => ($_SESSION['view_page']), //need to be in ()
        ),
    );
    /*
    $form['actions']['checkout'] = array(
      '#type' => 'submit',
      '#name' => 'checkout',
      '#value' => t('Checkout'),
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('checkout-button'),
      ),
    );
    */
    // switch off ajax for :
    $form['actions']['polypeptides'] = array(
        '#type' => 'submit',
        '#name' => 'polypeptides',
        '#value' => t('Polypeptides'),
        //'#ajax' => $ajax_settings,
        '#attributes' => array(
            'class' => array('checkout-button'),
        ),
    );
    $form['actions']['mrna'] = array(
        '#type' => 'submit',
        '#name' => 'mrna',
        '#value' => t('mRNA'),
        //'#ajax' => $ajax_settings,
        '#attributes' => array(
            'class' => array('checkout-button'),
        ),
    );
    $form['actions']['gene'] = array(
      '#type' => 'submit',
      '#name' => 'gene',
      '#value' => t('Gene'),
      '#attributes' => array(
        'class' => array('checkout-button'),
      ),
    );
    $form['actions']['update'] = array(
      '#type' => 'submit',
      '#submit' => array('_geneshop_cart_basket_update'),
      '#value' => t('Remove'),
//      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('update-button'),
      ),
    );
    $form['#validate'][] = 'geneshop_cart_check_empty_basket';
    $form['#attached'] = array(
      'css' => array(
        drupal_get_path('module', 'geneshop_cart') . '/theme/geneshop_cart.css',
      ),
      'js' => array(
        drupal_get_path('module', 'geneshop_cart') . '/theme/geneshop_cart.js',
      ),
    );
  }
  return $form;
}
/**
 * Basket page form qty element validate callback.
 */
function _geneshop_cart_qty_element_validate($element, &$form_state, $form) {
  if (!filter_var($element['#value'], FILTER_VALIDATE_INT) || $element['#value'] <= 0) {
    form_error($element, t('Incorrect value of qty'));
  }
}
/**
 * Basket page form submit callback.
 */
function geneshop_cart_basket_form_submit($form, &$form_state) {
  switch ($form_state['triggering_element']['#name']) {
    case 'clear':
      $form_state['redirect'] = 'basket/clear';
      break;
      /*
    case 'checkout':
      $form_state['redirect'] = 'basket/checkout';
      break;
      */
    case 'polypeptides':
      $form_state['redirect'] = 'basket/polypeptides';
      break;
    case 'mrna':
      $form_state['redirect'] = 'basket/mrna';
      break;
    case 'gene':
      $form_state['redirect'] = 'basket/gene';
     break;
  }
}
/**
 * Handling data update form the basket page.
 */
function _geneshop_cart_basket_update($form, &$form_state) {
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
    elseif (variable_get('geneshop_cart_show_qty', TRUE) && $_SESSION['basket']['items'][$key]['qty'] != $val['qty']) {
      $_SESSION['basket']['items'][$key]['qty'] = $val['qty'];
      if (variable_get('geneshop_cart_show_price', FALSE)) {
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
    _geneshop_cart_update_total();
  }
}
/**
 * Updating total sum of basket items.
 */
function _geneshop_cart_update_total() {
  if (empty($_SESSION['basket']['items'])) {
    unset($_SESSION['basket']);
  }
  elseif (variable_get('geneshop_cart_show_price', FALSE) && variable_get('geneshop_cart_price_field', '')) {
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
function geneshop_cart_checkout($ajax = FALSE) {
  module_load_include('inc', 'node', 'node.pages');
  $add_form = node_add('geneshop_cart_order');
  return $ajax ? $add_form : drupal_render($add_form);
}
*/

/**
 * Retrieves the sequences for a given feature. If a feature has multiple alignments
 *  or multiple relationships then multiple sequences will be returned.
 *
 * @param $feature
 *   An associative array describing the feature. Valid keys include:
 *    - feature_id: The feature_id of the feature for which the sequence will be retrieved
 *    - name: The feature name. This will appear on the FASTA definition line
 *    - parent_id:  (optional) only retrieve a sequence if 'derive_from_parent' is true 
 *        and the parent matches this ID.
 *    - featureloc_id: (optional) only retrieve a sequence if 'derive_from_parent' is 
 *        true and the alignment is defined with this featureloc_id
 * @param $options
 *   An associative array of options. Valid keys include:
 *    - width: Indicate the number of bases to use per line.  A new line will be added
 *        after the specified number of bases on each line.
 *    - is_html: Set to '1' if  the sequence is meant to be displayed on a web page.
 *        This will cause a <br> tag to separate lines of the FASTA sequence. 
 *    - derive_from_parent: Set to '1' if the sequence should be obtained from the parent
 *        to which this feature is aligned.
 *    - aggregate: Set to '1' if the sequence should only contain sub features, excluding
 *        intro sub feature sequence.  For example, set this option to obtain just
 *        the coding sequence of an mRNA.
 *    - upstream: An integer specifing the number of upstream bases to include in the output
 *    - downstream: An integer specifying the number of downstream bases to include in the
 *        output.
 *    - sub_feature_types: Only include sub features (or child features) of the types
 *        provided in the array
 *    - relationship_type: If a relationship name is provided (e.g. sequence_of) then any
 *        sequences that are in relationships of this type with matched sequences are also included
 *    - relationship_part: If a relationship is provided in the preceeding argument then
 *        the rel_part must be either 'object' or 'subject' to indicate which side of the
 *        relationship the matched features belong
 *
 * @return
 *   an array of matching sequence in the following keys for each sequence:
 *      'types'         => an array of feature types that were used to derive the sequence (e.g. from an aggregated sequence)
 *      'upstream'      => the number of upstream bases included in the sequence
 *      'downstream'    => the number of downstream bases included in the sequence
 *      'defline'       => the definintion line used to create a FASTA sequence
 *      'residues'      => the residues
 *      'featureloc_id' => the featureloc_id if the sequences is from an alignment
 *
 * @ingroup tripal_feature_api
 */
/*function tripal_get_feature_sequences($feature, $options) {

  // default values for finding the feature
  $feature_id         = array_key_exists('feature_id', $feature)     ? $feature['feature_id']     : 0;
  $parent_id          = array_key_exists('parent_id', $feature)      ? $feature['parent_id']      : 0;
  $featureloc_id      = array_key_exists('featureloc_id', $feature)  ? $feature['featureloc_id']  : 0;
  $feature_name       = array_key_exists('name', $feature)           ? $feature['name']           : '';

  // default values for building the sequence
  $num_bases_per_line = array_key_exists('width', $options)              ? $options['width']              : 50;
  $derive_from_parent = array_key_exists('derive_from_parent', $options) ? $options['derive_from_parent'] : 0;
  $aggregate          = array_key_exists('aggregate', $options)          ? $options['aggregate']          : 0;
  $upstream           = array_key_exists('upstream', $options)           ? $options['upstream']           : 0;
  $downstream         = array_key_exists('downstream', $options)         ? $options['downstream']         : 0;
  $sub_features       = array_key_exists('sub_feature_types', $options)  ? $options['sub_feature_types']  : array();
  $relationship       = array_key_exists('relationship_type', $options)  ? $options['relationship_type']  : '';
  $rel_part           = array_key_exists('relationship_part', $options)  ? $options['relationship_part']  : '';
  $is_html            = array_key_exists('is_html', $options)            ? $options['is_html']            : 0;
  
  // make sure the sub_features variable is an array
  if (!is_array($sub_features)) {
    tripal_report_error('tripal_feature', TRIPAL_ERROR,
      "'sub_features' option must be an array for function tripal_get_feature_sequences().",
      array()
    );
    return array();
  }

  // if a relationship was specified then retreive and the sequences that
  // have the given relationship and the recurse to extract the appropriate 
  // sequence
  if ($rel_part == "object" or $rel_part == "subject") {
    if ($rel_part == "subject") {
      $sql = '
        SELECT FO.feature_id, FO.name, FO.uniquename, CVTO.name as feature_type, O.genus, O.species
        FROM {feature} FS
          INNER JOIN {feature_relationship} FR ON FR.subject_id   = FS.feature_id
          INNER JOIN {cvterm} CVTFR            ON CVTFR.cvterm_id = FR.type_id
          INNER JOIN {feature} FO              ON FO.feature_id   = FR.object_id
          INNER JOIN {cvterm} CVTO             ON CVTO.cvterm_id  = FO.type_id
          INNER JOIN {organism} O              ON O.organism_id   = FO.organism_id
        WHERE
          FS.feature_id = :feature_id AND
          CVTFR.name    = :relationship
      ';
      $features = chado_query($sql, array(':feature_id' => $feature_id, ':relationship' => $relationship));
    }
    if ($rel_part == "object") {
      $sql = '
        SELECT FS.feature_id, FS.name, FS.uniquename, CVTO.name as feature_type, O.genus, O.species
        FROM {feature} FO
          INNER JOIN {feature_relationship} FR ON FR.object_id    = FO.feature_id
          INNER JOIN {cvterm} CVTFR            ON CVTFR.cvterm_id = FR.type_id
          INNER JOIN {feature} FS              ON FS.feature_id   = FR.subject_id
          INNER JOIN {cvterm} CVTO             ON CVTO.cvterm_id  = FS.type_id
          INNER JOIN {organism} O              ON O.organism_id   = FS.organism_id
        WHERE
          FO.feature_id = :feature_id AND
          CVTFR.name    = :relationship
      ';
      $features = chado_query($sql, array(':feature_id' => $feature_id, ':relationship' => $relationship));
    }
    $sequences = '';
    while ($feature = $features->fetchObject()) {

      // recurse and get the sequences for these in the relationship
      if ($rel_part == "subject") {
        $defline = "$feature_name, $relationship, $feature->uniquename $feature->feature_type ($feature->genus $feature->species)";
      }
      if ($rel_part == "object") {
        $defline = "$feature->uniquename $feature->feature_type ($feature->genus $feature->species), $relationship, $feature_name";
      }
      return tripal_get_feature_sequences(
        array(
          'feature_id' => $feature->feature_id, 
          'name' => $defline,
          'parent_id' => $parent_id,
        ),
        array(
          'width' => $num_bases_per_line, 
          'derive_from_parent' => $derive_from_parent, 
          'aggregate' => $aggregate, 
          'upstream' => $upstream, 
          'downstream' => $downstream, 
          'sub_features' => $sub_features, 
        )
      );
    }
  }

  // prepare the queries we're going to use later during the render phase
  // This SQL statement uses conditionals in the select clause to handle
  // cases cases where the alignment is in the reverse direction and when
  // the upstream and downstream extensions go beyond the lenght of the
  // parent sequence.
  $parent_sql ='
    SELECT featureloc_id, srcname, srcfeature_id, strand, srctypename, typename,
      fmin, fmax, upstream, downstream, adjfmin, adjfmax,
      substring(residues from (adjfmin + 1) for (upstream + (fmax - fmin) + downstream))  as residues,
      genus, species
    FROM (
      SELECT
        FL.featureloc_id, OF.name srcname, FL.srcfeature_id, FL.strand,
        OCVT.name as srctypename, SCVT.name as typename,
        FL.fmin, FL.fmax, OO.genus, OO.species,
        CASE
          WHEN FL.strand >= 0 THEN
            CASE
               WHEN FL.fmin - :upstream <= 0 THEN 0
               ELSE FL.fmin - :upstream
            END
          WHEN FL.strand < 0 THEN
            CASE
               WHEN FL.fmin - :downstream <= 0 THEN 0
               ELSE FL.fmin - :downstream
            END
        END as adjfmin,
        CASE
          WHEN FL.strand >= 0 THEN
            CASE
              WHEN FL.fmax + :downstream > OF.seqlen THEN OF.seqlen
              ELSE FL.fmax + :downstream
            END
          WHEN FL.strand < 0 THEN
            CASE
              WHEN FL.fmax + :upstream > OF.seqlen THEN OF.seqlen
              ELSE FL.fmax + :upstream
            END
        END as adjfmax,
        CASE
          WHEN FL.strand >= 0 THEN
            CASE
               WHEN FL.fmin - :upstream <= 0 THEN FL.fmin
               ELSE :upstream
            END
          ELSE
            CASE
               WHEN FL.fmax + :upstream > OF.seqlen THEN OF.seqlen - FL.fmax
               ELSE :upstream
            END
        END as upstream,
        CASE
          WHEN FL.strand >= 0 THEN
            CASE
               WHEN FL.fmax + :downstream > OF.seqlen THEN OF.seqlen - FL.fmax
               ELSE :downstream
            END
          ELSE
            CASE
               WHEN FL.fmin - :downstream <= 0 THEN FL.fmin
               ELSE :downstream
            END
        END as downstream,
        OF.residues
      FROM {featureloc} FL
        INNER JOIN {feature} SF   on FL.feature_id    = SF.feature_id
        INNER JOIN {cvterm}  SCVT on SF.type_id       = SCVT.cvterm_id
        INNER JOIN {feature} OF   on FL.srcfeature_id = OF.feature_id
        INNER JOIN {cvterm}  OCVT on OF.type_id       = OCVT.cvterm_id
        INNER JOIN {organism} OO  on OF.organism_id   = OO.organism_id
      WHERE SF.feature_id = :feature_id and NOT (OF.residues = \'\' or OF.residues IS NULL)) as tbl1
  ';
  // this query is meant to get all of the sub features of any given
  // feature (arg #1) and order them as they appear on the reference
  // feature (arg #2).
  $sfsql = '
    SELECT SF.feature_id, CVT.name as type_name, SF.type_id
    FROM {feature_relationship} FR
      INNER JOIN {feature} SF    ON SF.feature_id = FR.subject_id
      INNER JOIN {cvterm} CVT    ON CVT.cvterm_id = SF.type_id
      INNER JOIN {featureloc} FL ON FL.feature_id = FR.subject_id
      INNER JOIN {feature} PF    ON PF.feature_id = FL.srcfeature_id
    WHERE FR.object_id = :feature_id and PF.feature_id = :srcfeature_id
    ORDER BY FL.fmin ASC
  ';
  // for counting the number of children
  $fsql ='
    SELECT count(*) as num_children
    FROM {feature_relationship} FR
      INNER JOIN {feature} SF    ON SF.feature_id = FR.subject_id
      INNER JOIN {cvterm} CVT    ON CVT.cvterm_id = SF.type_id
      INNER JOIN {featureloc} FL ON FL.feature_id = FR.subject_id
      INNER JOIN {feature} PF    ON PF.feature_id = FL.srcfeature_id
    WHERE FR.object_id = :feature_id and PF.feature_id = :srcfeature_id
  ';

  // the array to be returned
  $sequences = array();
  
  // if we need to get the sequence from the parent then do so now.
  if ($derive_from_parent) {

    // execute the query to get the sequence from the parent
    $parents = chado_query($parent_sql, array(':upstream' => $upstream, ':downstream' => $downstream, ':feature_id' => $feature_id));
    while ($parent = $parents->fetchObject()) {
      
      // if the user specified a particular parent and this one doesn't match then skip it
      if ($parent_id and $parent_id != $parent->srcfeature_id) {
        continue;
      }
      // if the user specified a particular featureloc_id and this one doesn't match then skip it
      if ($featureloc_id and $featureloc_id != $parent->featureloc_id) {
        continue;
      }
      $seq = '';  // initialize the sequence for each parent
      $notes = '';

      // if we are to aggregate then we will ignore the feature returned
      // by the query above and rebuild it using the sub features
      if ($aggregate) {

        // now get the sub features that are located on the parent.
        $children = chado_query($sfsql, array(':feature_id' => $feature_id, ':srcfeature_id' => $parent->srcfeature_id));
        $num_children = chado_query($fsql, array(':feature_id' => $feature_id, ':srcfeature_id' => $parent->srcfeature_id))->fetchField();

        // iterate through the sub features and concat their sequences. They
        // should already be in order.
        $types = array();
        $i = 0;
        while ($child = $children->fetchObject()) {
          // if the callee has specified that only certain sub features should be
          // included then continue if this child is not one of those allowed
          // subfeatures
          if (count($sub_features) > 0 and !in_array($child->type_name, $sub_features)) {
            $i++;
             continue;
          }

          // keep up with the types
          if (!in_array($child->type_name, $types)) {
            $types[] = $child->type_name;
          }

          // if the first sub feature we need to include the upstream bases. first check if
          // the feature is in the foward direction or the reverse.
          if ($i == 0 and $parent->strand >= 0) {  // forward direction
            // -------------------------- ref
            //    ....---->  ---->
            //     up    1       2
            $q = chado_query($parent_sql, array(':upstream' => $upstream, ':downstream' => 0, ':feature_id' => $child->feature_id));
          }
          elseif ($i == 0 and $parent->strand < 0) { // reverse direction
            // -------------------------- ref
            //    ....<----  <----
            //    down  1       2
            $q = chado_query($parent_sql, array(':upstream' => 0, ':downstream' => $downstream, ':feature_id' => $child->feature_id));
          }

          // Next, if the last sub feature we need to include the downstream bases. first check if
          // the feature is in teh forward direction or the reverse
          elseif ($i == $num_children - 1 and $parent->strand >= 0) {  // forward direction
            // -------------------------- ref
            //        ---->  ---->....
            //          1       2 down
            $q = chado_query($parent_sql, array(':upstream' => 0, ':downstream' => $downstream, ':feature_id' => $child->feature_id));
          }
          elseif ($i == $num_children - 1 and $parent->strand < 0) { // reverse direction
            // -------------------------- ref
            //        <----  <----....
            //          1       2  up
            $q = chado_query($parent_sql, array(':upstream' => $upstream, ':downstream' => 0, ':feature_id' => $child->feature_id));
          }
          // for internal sub features we don't want upstream or downstream bases
          else {
            $q = chado_query($parent_sql, array(':upstream' => 0, ':downstream' => 0, ':feature_id' => $child->feature_id));
          }
          while ($subseq = $q->fetchObject()) {
            // concatenate the sequences of all the sub features
            if ($subseq->srcfeature_id == $parent->srcfeature_id) {
              $seq .= $subseq->residues;
            }
            if ($subseq->upstream > 0 ) { 
              $notes .= "Includes " . $subseq->upstream . " bases upstream.  ";
            }
            if ($subseq->downstream > 0) {
              $notes .= "Includes " . $subseq->downstream . " bases downstream.  ";
            }
          }
          $i++;
        }
      }
      // if this isn't an aggregate then use the parent residues
      else {
         $seq = $parent->residues;
         if ($parent->upstream > 0) {
           $notes .= "Includes " . $parent->upstream . " bases upstream.  ";
         }
         if ($parent->downstream > 0) {
           $notes .= "Includes " . $parent->downstream . " bases downstream.  ";
         }
      }

      // get the reverse compliment if feature is on the reverse strand
      $dir = 'forward';
      $length = strlen($seq);
      if ($parent->strand < 0) {
        $seq = tripal_reverse_compliment_sequence($seq);
        $dir = 'reverse';
      }
      // now format for display
      if ($is_html) {
        $seq = wordwrap($seq, $num_bases_per_line, "<br>", TRUE);
      }
      else {
        $seq = wordwrap($seq, $num_bases_per_line, "\r\n", TRUE);
      }
      if (!$seq) {
        $notes .= "No sequence available.";
      }

      if (count($types) > 0) {
        $notes .= "Excludes all bases but those of type(s): " . implode(', ', $types) . ". " ;
      }

      // construct the definition line for this feature.
      // to construct the defline we need a featureloc record, so we'll create one using
      // the information we have
      $featureloc = new stdClass;
      $featureloc->feature_id = $feature;
      $featureloc->fmin = $parent->adjfmin;
      $featureloc->fmax = $parent->adjfmax;
      $featureloc->strand = $parent->strand;
      $featureloc->srcfeature_id = new stdClass;
      $featureloc->srcfeature_id->name = $parent->srcname;
      $featureloc->srcfeature_id->type_id = $parent->srctypename;
      $featureloc->srcfeature_id->organism_id = new stdClass;
      $featureloc->srcfeature_id->organism_id->genus = $parent->genus;
      $featureloc->srcfeature_id->organism_id->species = $parent->species;
      // get a proper feature object
      $f = chado_generate_var('feature', array('feature_id' => $feature_id));
      $defline = tripal_get_fasta_defline($f, $notes, $featureloc, '', $length);
      
      $sequences[] = array(
        'types'         => $types,
        'upstream'      => $parent->upstream,
        'downstream'    => $parent->downstream,
        'defline'       => $defline,
        'residues'      => $seq,
        'featureloc_id' => $parent->featureloc_id,
        'length'        => $length,
      );
    }
  }
  // if we are not getting the sequence from the parent sequence then
  // use what comes through from the feature record
  else {
    $sql = "SELECT * FROM {feature} F WHERE feature_id = :feature_id";
    $values = chado_query($sql, array(':feature_id' => $feature_id))->fetchObject();
    $residues = $values->residues;
    $length = strlen($residues);
    if ($is_html) {
      $residues = wordwrap($residues, $num_bases_per_line, "<br>", TRUE);
    }
    else {
      $residues = wordwrap($residues, $num_bases_per_line, "\r\n", TRUE);
    }
    
    // get the definintion line for this feature
    $f = chado_generate_var('feature', array('feature_id' => $feature_id));
    $defline = tripal_get_fasta_defline($f, '', NULL, '', $length);

    // add to the sequence array
    $sequences[] = array(
      'types'      => $values->type,
      'upstream'   => 0,
      'downstream' => 0,
      'defline'    => $defline,
      'residues'   => $residues,
      'length'     => $length,
    );
  }

  return $sequences;
}*/

function geneshop_cart_polypeptides() {
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

function geneshop_cart_mrna($ajax = FALSE) {
    $sql =<<<SQL
            select f.name, f.residues
            from chado.gene g,
            chado.feature_relationship fr,
            chado.feature f,
            chado.cvterm cvt,
                 public.chado_feature chf
            where g.gene_id = fr.object_id
                  and fr.subject_id = f.feature_id
                  and g.gene_id = chf.feature_id
                  and f.type_id = cvt.cvterm_id
                  and cvt.name = 'mRNA'
            and chf.nid = :nid
SQL;
    $regex = "/([acgtACGT])/";
    generate_fasta($sql, $regex);
}

function geneshop_cart_gene() {
  $sql =<<<SQL
           select f.name, f.residues
           from chado.gene g,
           chado.feature f,
           chado.cvterm cvt,
                public.chado_feature chf
           where g.gene_id = chf.feature_id
                 and f.type_id = cvt.cvterm_id
                 and cvt.name = 'gene'
                 and chf.nid = :nid
SQL;
    $regex = "/([acgtACGT])/";
    generate_fasta($sql, $regex);
}

/**
 * Confirmation form when cleaning basket.
 */
function _geneshop_cart_clear_confirm($form, &$form_state) {
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
function _geneshop_cart_clear_confirm_submit($form, &$form_state) {
  unset($_SESSION['basket']);
  drupal_set_message(t('Your basket is cleared'));
  drupal_goto($_SESSION['view_page']);
}
/**
 * Helper function to check the access code when referring to a specific order.
 */
function _geneshop_cart_get_my_order($node) {
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
function geneshop_cart_my_orders_page($account) {
  drupal_set_title(t("@name's Order History", array('@name' => format_username($account))), PASS_THROUGH);
  $build = array();
  $query = db_select('node', 'n')->extend('PagerDefault');
  $nids = $query->fields('n', array('nid', 'sticky', 'created'))->condition('type', geneshop_cart_ORDER_NODETYPE)->condition('uid', $account->uid)->condition('status', 1)->orderBy('created', 'DESC')->limit(variable_get('default_nodes_main', 10))->addTag('node_access')->execute()->fetchCol();
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