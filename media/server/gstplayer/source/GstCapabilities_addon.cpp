/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

std::vector<std::string> GstCapabilities::getSupportedProperties(MediaSourceType mediaType,
                                                                 const std::vector<std::string> &propertyNames)
{
    // Get gstreamer element factories. The following flag settings will fetch both SINK and DECODER types
    // of gstreamer classes...
    GstElementFactoryListType factoryListType{GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER};
    {
        // If MediaSourceType::AUDIO is specified then adjust the flag so that we
        // restrict the list to gstreamer AUDIO element types (and likewise for video and subtitle)...
        static const std::unordered_map<MediaSourceType, GstElementFactoryListType>
            kLookupExtraConditions{{MediaSourceType::AUDIO, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO},
                                   {MediaSourceType::VIDEO, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO},
                                   {MediaSourceType::SUBTITLE, GST_ELEMENT_FACTORY_TYPE_MEDIA_SUBTITLE}};
        auto i = kLookupExtraConditions.find(mediaType);
        if (i != kLookupExtraConditions.end())
            factoryListType |= i->second;
    }

    GList *factories{m_gstWrapper->gstElementFactoryListGetElements(factoryListType, GST_RANK_NONE)};

    // Scan all returned elements for the specified properties...
    std::unordered_set<std::string> propertiesToLookFor{propertyNames.begin(), propertyNames.end()};
    std::vector<std::string> propertiesFound;
    for (GList *iter = factories; iter != nullptr && !propertiesToLookFor.empty(); iter = iter->next)
    {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(iter->data);
        gpointer elementClass{nullptr};
        GstPluginFeature *loadFeature{nullptr};
        GstElement *elementObj{nullptr};

        RIALTO_SERVER_LOG_DEBUG("Searching the properties of a class");
        {
            // Try to obtain the class without instantiating an object...
            GType elementType = m_gstWrapper->gstElementFactoryGetElementType(factory);
            if (elementType == G_TYPE_INVALID)
            {
                RIALTO_SERVER_LOG_DEBUG("  Using gstPluginFeatureLoad");
                loadFeature = m_gstWrapper->gstPluginFeatureLoad(GST_PLUGIN_FEATURE(factory));
                if (loadFeature)
                    elementType = m_gstWrapper->gstElementFactoryGetElementType(factory);
            }

            if (elementType != G_TYPE_INVALID)
                elementClass = m_glibWrapper->gTypeClassRef(elementType);
        }

        if (!elementClass)
        {
            // Create an object because we couldn't get the class
            RIALTO_SERVER_LOG_DEBUG("  Using gstElementFactoryCreate");
            elementObj = m_gstWrapper->gstElementFactoryCreate(factory, nullptr);
        }

        if (elementClass || elementObj)
        {
            GParamSpec **props;
            guint nProps;
            if (elementObj)
                props = m_glibWrapper->gObjectClassListProperties(G_OBJECT_GET_CLASS(elementObj), &nProps);
            else
                props = m_glibWrapper->gObjectClassListProperties(G_OBJECT_GET_CLASS(elementClass), &nProps);

            if (!props && elementClass && !loadFeature)
            {
                RIALTO_SERVER_LOG_DEBUG("  No properties seen, trying gst_plugin_feature_load");
                m_glibWrapper->gObjectUnref(elementClass);
                elementClass = nullptr;
                loadFeature = m_gstWrapper->gstPluginFeatureLoad(GST_PLUGIN_FEATURE(factory));
                if (loadFeature)
                {
                    GType elementType = m_gstWrapper->gstElementFactoryGetElementType(factory);

                    if (elementType != G_TYPE_INVALID)
                        elementClass = m_glibWrapper->gTypeClassRef(elementType);
                    if (elementClass)
                    {
                        props = m_glibWrapper->gObjectClassListProperties(G_OBJECT_GET_CLASS(elementClass), &nProps);
                        if (props)
                        {
                            RIALTO_SERVER_LOG_DEBUG("  gst_plugin_feature_load worked");
                        }
                    }
                }
            }

            if (!props && !elementObj)
            {
                // Fall back to create an object
                RIALTO_SERVER_LOG_DEBUG("  No properties seen, creating an object");
                elementObj = m_gstWrapper->gstElementFactoryCreate(factory, nullptr);
                if (elementObj)
                    props = m_glibWrapper->gObjectClassListProperties(G_OBJECT_GET_CLASS(elementObj), &nProps);
            }

            if (props)
            {
                for (guint j = 0; j < nProps && !propertiesToLookFor.empty(); ++j)
                {
                    const std::string kPropName{props[j]->name};
                    auto it = propertiesToLookFor.find(kPropName);
                    if (it != propertiesToLookFor.end())
                    {
                        RIALTO_SERVER_LOG_DEBUG("Found property '%s'", kPropName.c_str());
                        propertiesFound.push_back(kPropName);
                        propertiesToLookFor.erase(it);
                    }
                }
                m_glibWrapper->gFree(props);
            }
        }
        if (elementObj)
            m_gstWrapper->gstObjectUnref(elementObj);
        if (loadFeature)
            m_gstWrapper->gstObjectUnref(loadFeature);
        if (elementClass)
            m_glibWrapper->gObjectUnref(elementClass);
    }

    m_gstWrapper->gstPluginFeatureListFree(factories);
    return propertiesFound;
}
