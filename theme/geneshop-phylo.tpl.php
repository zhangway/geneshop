<?php

$phylo = '';
$list_items = array();

$list_items[] = '<a name="Phylogenetic Tree from Clustal Omega"></a>';
$phylo_html .= '<div id="Name of Phylo" class="geneshop-item">';
$phylo_html .= '<pre class="geneshop-phylo">';
$phylo_html .= $phylo;
$phylo_html .= '</pre>';
$phylo_html .= '</div>';
?>

<?php

print theme_item_list(array(
  'items' => $list_items,
  'title' => '',
  'type' => 'ul',
  'attributes' => array(),
));

print $phylo_html;
