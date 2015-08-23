<?php
/**
 * @file
 * Interface for downloading feature sequences
 */

/**
 * The page allowing users to download feature sequences
 *
 * @ingroup tripal_feature
 */
function geneshop_cart_seq_extract_page() {

  if ($_SESSION['geneshop_cart_seq_extract']['download']) {
    $genus      = $_SESSION['geneshop_cart_seq_extract']['genus'];
    $species    = $_SESSION['geneshop_cart_seq_extract']['species'];
    $analysis   = $_SESSION['geneshop_cart_seq_extract']['analysis'];
    $ftype      = $_SESSION['geneshop_cart_seq_extract']['ftype'];
    $fnames     = $_SESSION['geneshop_cart_seq_extract']['fnames'];
    $upstream   = $_SESSION['geneshop_cart_seq_extract']['upstream'];
    $downstream = $_SESSION['geneshop_cart_seq_extract']['downstream'];
    $format     = $_SESSION['geneshop_cart_seq_extract']['format'];
    $use_parent = $_SESSION['geneshop_cart_seq_extract']['use_parent'];
    $aggregate  = $_SESSION['geneshop_cart_seq_extract']['aggregate'];
    $agg_types  = $_SESSION['geneshop_cart_seq_extract']['agg_types'];

    unset($_SESSION['geneshop_cart_seq_extract']['download']);

    if ($format == 'fasta_html') {
      drupal_add_http_header('Content-Type: text/html');
    }
    else {
      drupal_add_http_header('Content-Type: text');
      drupal_add_http_header('Content-Disposition: attachment; filename="sequences.fasta.txt"');
    }

    tripal_get_bulk_feature_sequences(array(
      'genus' => $genus, 
      'species' => $species, 
      'analysis_name' => $analysis,
      'type' => $ftype, 
      'feature_name' => $fnames['items_array'], 
      'upstream' => $upstream, 
      'downstream' => $downstream, 
      'print' => 1,
      'output_format' => $format, 
      'derive_from_parent' => $use_parent, 
      'aggregate' => $aggregate,
      'sub_feature_types' => $agg_types
    ));

    return;
  }


  // generate the search form
  $output .= '';
  if (user_access('administer tripal')) {
    $output .= tripal_set_message("Administrators, the " . 
        l('organism_feature_count', 'admin/tripal/schema/mviews') . " and " .
        l('analysis_organism', 'admin/tripal/schema/mviews') . " materialized
        views must be populated before using this form.  Those views should be re-populated
        when new data is added.");
  }
  $output .= "<div id=\"tripal-feature-seq-extract-form-block\">";
  $output .= drupal_get_form('tripal_feature_seq_extract_form');
  $output .= "</div>";
  return $output;
}
