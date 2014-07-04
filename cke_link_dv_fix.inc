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
  // Use strpos() to avoid excessive regex calls.
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
    // @todo (alex): it's okay only for FC, the logic should be updated for
    // other projects.
    $correct_prefix = empty($GLOBALS['_domain_variant']->path)
        ? $GLOBALS['base_path']
        : '//' . $GLOBALS['_domain_variant']->path . '/';
    $prefixes = db_select('domain_variants', 'dv')
      ->fields('dv', array('path'))
      ->execute()
      ->fetchCol();
    $prefix_to_len_map = array();
    foreach ($prefixes as $prefix) {
      foreach (array('http://', 'https://') as $protocol) {
        $_prefix = $protocol . $prefix . '/';
        $prefix_to_len_map[$_prefix] = strlen($_prefix);
      }
    }
    // Base path should be the last one.
    $prefix_to_len_map[$GLOBALS['base_path']] = strlen($GLOBALS['base_path']);
  }
  foreach ($prefix_to_len_map as $prefix => $len) {
    if (strpos($matches[1], $prefix) === 0) {
      $correct = substr_replace($matches[1], $correct_prefix, 0, $len);
      return 'href="' . $correct;
    }
  }
  return $matches[0];
}

/**
 * Searches an entity for the formatted text fields.
 *
 * @param object $entity
 * @param string $entity_type
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
function &_cke_link_dv_fix_find_filtered_text_fields(&$entity, $entity_type) {
  $return = array();
  list( , , $bundle) = entity_extract_ids($entity_type, $entity);
  $filtered_text_fields = _cke_link_dv_fix_filtered_text_field_list($entity_type, $bundle);
  foreach ($entity as $field_name => $_) {
    if (isset($filtered_text_fields[$field_name]) && !empty($entity->{$field_name})) {
      // Process only for current language.
      $language = field_language($entity_type, $entity, $field_name);
      if (!empty($entity->{$field_name}[$language])) {
        foreach ($entity->{$field_name}[$language] as $delta => $__) {
          $return[] =& $entity->{$field_name}[$language][$delta];
        }
      }
    }
  }
  return $return;
}

/**
 * Returns an array of field names with text processing enabled.
 *
 * @param string $entity_type
 * @param string $bundle
 *
 * @return array
 */
function _cke_link_dv_fix_filtered_text_field_list($entity_type, $bundle) {
  static $cache;
  if ($cache == NULL) {
    $cache = field_info_instances();
    foreach ($cache as $_entity_type => $bundles) {
      foreach ($bundles as $_bundle => $instances) {
        foreach ($instances as $field_name => $instance) {
          if (empty($instance['settings']['text_processing'])) {
            unset($cache[$_entity_type][$_bundle][$field_name]);
          }
          else {
            $cache[$_entity_type][$_bundle][$field_name] = $field_name;
          }
        }
      }
    }
  }
  return $cache[$entity_type][$bundle];
}
