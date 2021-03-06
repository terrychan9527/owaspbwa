<?php
/*
 * $RCSfile: ItemAdmin.inc,v $
 *
 * Gallery - a web based photo album viewer and editor
 * Copyright (C) 2000-2006 Bharat Mediratta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @version $Revision: 1.52 $ $Date: 2006/01/10 04:39:20 $
 * @package GalleryCore
 * @subpackage UserInterface
 * @author Bharat Mediratta <bharat@menalto.com>
 */

/**
 * This view will present the available options to administrate an item/album
 *
 * @package GalleryCore
 * @subpackage UserInterface
 *
 */
class ItemAdminView extends GalleryView {

    /**
     * @see GalleryView::loadTemplate
     */
    function loadTemplate(&$template, &$form) {
	global $gallery;

	/* Figure out the item we're referring to */
	$itemId = GalleryUtilities::getRequestVariables('itemId');
	list ($ret, $item) = GalleryCoreApi::loadEntitiesById($itemId);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Find its thumbnail */
	list ($ret, $thumbnailTable) = GalleryCoreApi::fetchThumbnailsByItemIds(array($itemId));
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}
	if (empty($thumbnailTable)) {
	    $thumbnailData = null;
	} else {
	    $thumbnail = $thumbnailTable[$itemId];
	    $thumbnailData = (array)$thumbnail;
	}

	/* Load the module list */
	list ($ret, $moduleStatus) = GalleryCoreApi::fetchPluginStatus('module');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Get a list of all the admin views for each module */
	$subViewChoiceData = array();
	foreach ($moduleStatus as $moduleId => $status) {
	    if (empty($status['active'])) {
		continue;
	    }

	    /* Get the selected module's admin view */
	    if (in_array('getItemAdminViews', explode('|', $status['callbacks']))) {
		list($ret, $module) = GalleryCoreApi::loadPlugin('module', $moduleId);
		if ($ret) {
		    if ($ret->getErrorCode() & ERROR_PLUGIN_VERSION_MISMATCH) {
			continue;
		    }
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		list ($ret, $moduleViews) = $module->getItemAdminViews($item);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		$subViewChoiceData = array_merge($subViewChoiceData, $moduleViews);
	    }
	}

	usort($subViewChoiceData, array($this, 'sortByName'));

	$subViewName = GalleryUtilities::getRequestVariables('subView');
	if ($subViewName == 'core.ItemAdmin') {
	    return array(GalleryCoreApi::error(ERROR_BAD_PARAMETER, __FILE__, __LINE__), null);
	}

	/* Turn sub view choices into url params */
	$subViewChoices = array();
	foreach ($subViewChoiceData as $params) {
	    /* Extract the view name */
	    $name = $params['name'];
	    unset($params['name']);

	    /* Move the view -> subView and set ItemAdmin as the view */
	    $params['subView'] = $params['view'];
	    $params['view'] = 'core.ItemAdmin';
	    if ($subViewName == $params['subView']) {
		$params['active'] = true;
	    }

	    /* Set the item id if necessary */
	    if (!isset($params['itemId'])) {
		$params['itemId'] = $item->getId();
	    }

	    $subViewChoices[$name] = $params;
	}

	/* If we have a specific sub view, render it now */
	if (empty($subViewName) && !empty($subViewChoices)) {
	    $subViewName = 'core.ItemEdit';
	}

	list ($ret, $subView) = GalleryView::loadView($subViewName);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Get the type of the item */
	$itemType = $item->itemTypeName(true);

	/* Load the item's parents */
	list ($ret, $parents) = GalleryCoreApi::fetchParents($item, 'core.view');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}
	for ($i = 0; $i < sizeof($parents); $i++) {
	    $parents[$i] = (array)$parents[$i];
	}

	list ($ret, $results) = $subView->loadTemplate($template, $form);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $isSiteAdmin) = GalleryCoreApi::isUserInSiteAdminGroup();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Our sub view may have set some hints, like the encoding type */
	if ($template->hasVariable('ItemAdmin')) {
	    $ItemAdmin =& $template->getVariableByReference('ItemAdmin');
	} else {
	    $ItemAdmin = array();
	}

	list ($ret, $rootAlbumId) =
	    GalleryCoreApi::getPluginParameter('module', 'core', 'id.rootAlbum');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Get the links for navigating back to where we came from */
	$urlGenerator =& $gallery->getUrlGenerator();
	list ($ret, $navigationLinks) = $urlGenerator->getNavigationLinks();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list($ret, $module) = GalleryCoreApi::loadPlugin('module', 'core');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$ItemAdmin['navigationLinks'] = $navigationLinks;
	$ItemAdmin['viewBodyFile'] = $results['body'];
	$ItemAdmin['item'] = (array)$item;
	$ItemAdmin['parents'] = $parents;
	$ItemAdmin['parent'] = 	empty($parents) ? null : $parents[sizeof($parents) - 1];
	$ItemAdmin['itemType'] = $itemType;
	$ItemAdmin['isRootAlbum'] = $item->getId() == $rootAlbumId;
	$ItemAdmin['thumbnail'] = $thumbnailData;
	$ItemAdmin['subViewChoices'] = $subViewChoices;
	$ItemAdmin['viewL10Domain'] = $subView->getL10Domain();
	$ItemAdmin['isSiteAdmin'] = $isSiteAdmin;

	if (!isset($ItemAdmin['enctype'])) {
	    $ItemAdmin['enctype'] = 'application/x-www-form-urlencoded';
	}

	$template->setVariable('ItemAdmin', $ItemAdmin);
	$template->title($module->translate('Gallery Item Administration'));
	return array(null,
		     array('body' => 'modules/core/templates/ItemAdmin.tpl'));
    }

    function sortByName($a, $b) {
	return strcmp($a['name'], $b['name']);
    }

    /**
     * @see GalleryView::getViewDescription()
     */
    function getViewDescription() {
	/* Get the description from the current subView */
	$subViewName = GalleryUtilities::getRequestVariables('subView');
	if (empty($subViewName)) {
	    $subViewName = 'core.ItemEdit';
	}
	list ($ret, $subView) = GalleryView::loadView($subViewName);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $description) = $subView->getViewDescription();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	return array(null, $description);
    }

    /**
     * @see GalleryView::getViewType()
     */
    function getViewType() {
	return VIEW_TYPE_ADMIN;
    }
}
?>
