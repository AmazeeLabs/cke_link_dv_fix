<?php

/**
 * A replacement for _ckeditor_link_filter_process().
 *
 * The only difference: it adds setcurrentdomainvariant="true" before the href.
 *
 * @see _ckeditor_link_filter_process()
 */
function _cke_link_dv_fix_ckeditor_link_filter_process($matches, $langcode = NULL) {
  static $stored_langcode = LANGUAGE_NONE;
  if ($matches === NULL) {
    $stored_langcode = $langcode;
    return;
  }
  $path = urldecode($matches[1]);
  $langcode = $stored_langcode;
  $path = ckeditor_link_path_strip_language($path, $langcode);
  $types = ckeditor_link_get_types();
  foreach ($types as $type) {
    $func = $type['module'] .'_ckeditor_link_'. $type['type'] .'_url';
    if (function_exists($func)) {
      $url = $func($path, $langcode);
      if ($url) {
        return 'setcurrentdomainvariant="true" href="'. $url;
      }
    }
  }
  return 'href="'. base_path() . $matches[1];
}

/**
 * Processes links replacing the cached domain variant with the current one.
 *
 * @param string $text
 */
function _cke_link_dv_fix_correct_domain_variants(&$text) {
  //AXXX fast
  if (strpos($text, 'setcurrentdomainvariant="true" href="') !== FALSE) {
    $text = preg_replace_callback('`setcurrentdomainvariant="true" href="([^?#"]+)`', '_cke_link_dv_fix_correct_domain_variants_callback', $text);
  }
}

/**
 * A callback for the preg_replace_callback().
 */
function _cke_link_dv_fix_correct_domain_variants_callback($matches) {
  static $init, $correct_prefix, $prefix_to_len_map;
  if ($init === NULL) {
    $init = 'done';
    $correct_prefix = '//' . $GLOBALS['_domain_variant']->path . '/';
    $prefixes = db_select('domain_variants', 'dv')
      ->fields('dv', array('path'))
      ->execute()
      ->fetchCol();
    $prefix_to_len_map = array();
    foreach ($prefixes as $prefix) {
      $prefix_to_len_map['//' . $prefix . '/'] = strlen($prefix) + 3;
    }
  }
  foreach ($prefix_to_len_map as $prefix => $len) {
    $pos = strpos($matches[1], $prefix);
    if ($pos !== FALSE) {
      $correct = substr_replace($matches[1], $correct_prefix, $pos, $len);
      return 'href="' . $correct;
    }
  }
  return $matches[0];
}

/**
 * Recursively searches an entity for the formatted text fields.
 *
 * @param object|array $entity
 *
 * @return array
 *   An indexed array containing formatted text field values, example:
 *   @code
 *   array(
 *     array('value' => '', 'format' => 'full_html', 'safe_value' => ''),
 *     ...,
 *   )
 *   @endcode
 *   The values are references.
 */
function &_cke_link_dv_fix_find_filtered_text_fields(&$entity) {
  $return = array();
  foreach ($entity as &$property) {
    if (is_array($property)) {
      if (isset($property['value']) && array_key_exists('format', $property) && isset($property['value'])) {
        if ($property['format'] !== NULL) {
          $return[] =& $property;
        }
      }
      else {
        $return = array_merge(_cke_link_dv_fix_find_filtered_text_fields($property), $return);
      }
    }
    elseif (is_object($property)) {
      $return = array_merge(_cke_link_dv_fix_find_filtered_text_fields($property), $return);
    }
  }
  return $return;
}
