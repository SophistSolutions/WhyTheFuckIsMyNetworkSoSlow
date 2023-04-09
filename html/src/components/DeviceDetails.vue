<script setup lang="ts">
import { onMounted, onUnmounted, Ref, ref, computed } from "vue";
import moment from "moment";
import { DateTime } from "luxon";
import JsonViewer from "vue-json-viewer";
import { PluralizeNoun } from "src/utils/Linguistics";
import { Notify } from "quasar";

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { ComputeServiceTypeIconURL } from "../models/device/Utils";
import {
  GetNetworkLink,
  GetNetworkName,
  GetServices,
  SortNetworks,
  FormatIDateTimeRange,
  FormatSeenMap,
} from "../models/network/Utils";
import * as proxyAPI from "../proxy/API";

// Components
import ReadOnlyTextWithHover from "../components/ReadOnlyTextWithHover.vue";
import Link2DetailsPage from "../components/Link2DetailsPage.vue";
import NetworkInterfacesDetails from "../components/NetworkInterfacesDetails.vue";
import PopupEditTextField from "../components/PopupEditTextField.vue";
import PopupEditTagsListField from "../components/PopupEditTagsListField.vue";

import { useNetStateStore } from "../stores/Net-State-store";
import { INetwork } from "src/models/network/INetwork";

import { patchDeviceUserProps_name, patchDeviceUserProps_notes, patchDeviceUserProps_tags } from "../proxy/API";

const store = useNetStateStore();

interface Props {
  // EITHER device or deviceId - EXCLUSIVE and ONE required
  device?: IDevice;
  deviceId?: string;
  includeLinkToDetailsPage?: boolean;
  showExtraDetails?: boolean;
  showOldNetworks?: boolean;
  showInactiveInterfaces?: boolean;
  showSeenDetails?: boolean;
  allowEdit?: boolean;
}
const props = withDefaults(defineProps<Props>(), {
  includeLinkToDetailsPage: false,
  showExtraDetails: false,
  showOldNetworks: false,
  showInactiveInterfaces: true,
  showSeenDetails: false,
  allowEdit: false,
});

let polling: undefined | NodeJS.Timeout;
var isRescanning: Ref<boolean> = ref(false);

let defaultDisplayedNameForPopup_ = computed<string>(() => {
  if (
    currentDevice.value?.userOverrides?.name &&
    currentDevice.value?.names?.length > 1
  ) {
    return currentDevice.value.names[1].name;
  } else if (
    currentDevice.value?.names?.length &&
    currentDevice.value?.names?.length > 0
  ) {
    return currentDevice.value.names[0].name;
  }
  return "";
});

function localNetworkAddresses(): string[] {
  const addresses: string[] = [];
  if (currentDevice.value) {
    Object.entries(currentDevice.value.attachedNetworks).forEach((element) => {
      element[1].localAddresses.forEach((e: string) => addresses.push(e));
    });
  }
  return addresses.filter((value, index, self) => self.indexOf(value) === index);
}

function doFetches() {
  if (!props.device) {
    store.fetchDevice(props.deviceId as string);
  }
  if (currentDevice.value) {
    store.fetchNetworks(Object.keys(currentDevice.value.attachedNetworks));
    if (currentDevice.value.aggregatesReversibly != null) {
      store.fetchDevices(currentDevice.value.aggregatesReversibly);
    }
  }
}

onMounted(() => {
  doFetches();
  // first time check immediately, then more gradually for updates
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches();
  }, 15 * 1000);
});

const editNamePopup = ref(null);
const editNotesPopup = ref(null);
const editTagsPopup = ref(null);

onUnmounted(() => {
  clearInterval(polling);
});

async function notifyOfDeviceNameEdit_(v: string|null) {
  if (currentDevice.value?.id) {
    try {
      await patchDeviceUserProps_name(currentDevice.value?.id, v);
      store.fetchDevices([currentDevice.value?.id]);
      Notify.create("updated");
    } catch (e) {
      Notify.create(`Failed updating device name: ${(e as any)?.message}!`);
    }
  }
}

async function notifyOfDeviceNotesEdit_(v: string|null) {
  if (currentDevice.value?.id) {
    try {
      await patchDeviceUserProps_notes(currentDevice.value?.id, v);
      store.fetchDevices([currentDevice.value?.id]);
      Notify.create("updated");
    } catch (e) {
      Notify.create(`Failed updating device notes: ${(e as any)?.message}!`);
    }
  }
}

async function notifyOfDeviceTagsEdit_(v: string[]|null) {
  if (currentDevice.value?.id) {
    try {
      await patchDeviceUserProps_tags(currentDevice.value?.id, v);
      store.fetchDevices([currentDevice.value?.id]);
      Notify.create('updated');
    } catch (e) {
      Notify.create(`Failed updating device tags: ${(e as any)?.message}!`);
    }
  }
}

function validateDeviceName_(v: any) {
  if (v) {
    return v.length >= 1;
  } else {
    return v === null; // allow null - meaning set to null, but undefined just means user made no changes
  }
}

async function rescanDevice(): Promise<void> {
  isRescanning.value = true;
  try {
    if (currentDevice?.value) {
      await proxyAPI.rescanDevice(currentDevice.value.id);
      store.fetchDevice(currentDevice.value.id);
    }
  } finally {
    isRescanning.value = false;
  }
}
async function editName(): Promise<void> {
  editNamePopup.value?.startEdit();
}
async function editNotes(): Promise<void> {
  editNotesPopup.value?.startEdit();
}
async function editTags(): Promise<void> {
  editTagsPopup.value?.startEdit();
}

defineExpose({
  rescanDevice,
  editName,
  editNotes,
  editTags,
});

const currentDevice = computed<IDevice | undefined>(
  () => props.device ?? store.getDevice(props.deviceId as string)
);

function SortDeviceIDsByMostRecentFirst_(ids: Array<string>): Array<string> {
  let r: Array<string> = ids.filter((x) => true);
  r.sort((l, r) => {
    let lSeen = store.getDevice(l)?.seen?.Ever;
    let rSeen = store.getDevice(r)?.seen?.Ever;
    if (lSeen?.upperBound == null) {
      return 1;
    }
    if (rSeen?.upperBound == null) {
      return -1;
    }
    return moment(rSeen.upperBound).diff(lSeen.upperBound);
  });
  return r;
}

function GetSubDeviceDisplay_(id: string, summaryOnly: boolean): string {
  let result: string = "";
  {
    let r = store.getDevice(id);
    if (r) {
      const seenText = FormatSeenMap(r.seen, summaryOnly);
      if (seenText) {
        result += seenText;
      }
    }
  }
  if (result.length == 0 || !summaryOnly) {
    result += "; ID: " + id;
  }
  return result;
}

interface IExtendedDevice extends IDevice {
  localAddresses: string;
  attachedNetworks: any;
  attachedFullNetworkObjects: INetwork[];
}

let currentDeviceDetails = computed<IExtendedDevice | undefined>(() => {
  if (currentDevice.value) {
    let attachedFullNetworkObjects = [] as INetwork[];
    let attachedNetworkInfo = {} as { [key: string]: object };
    Object.keys(currentDevice.value.attachedNetworks).forEach((element: any) => {
      const thisNWI = currentDevice.value.attachedNetworks[
        element
      ] as INetworkAttachmentInfo;
      let netName = "?";
      const thisNetObj: INetwork = store.getNetwork(element);
      if (thisNetObj) {
        netName = GetNetworkName(thisNetObj);
        attachedFullNetworkObjects.push(thisNetObj);
      }
      attachedNetworkInfo[element] = { ...thisNWI, name: netName };
    });
    attachedFullNetworkObjects = SortNetworks(attachedFullNetworkObjects);
    return {
      ...currentDevice.value,
      ...{
        localAddresses: localNetworkAddresses().join(", "),
        attachedNetworks: attachedNetworkInfo,
        attachedFullNetworkObjects: attachedFullNetworkObjects,
      },
    };
  }
  return undefined;
});

const aliases = computed<string[] | undefined>(() => {
  // priority 0 names are all terrible - basically IP addresses
  return currentDevice.value?.names
    ?.filter((m) => m.priority > 1)
    .map((m) => m.name)
    .slice(1);
});
</script>

<template>
  <div v-if="currentDevice" class="q-pa-sm">
    <div class="row">
      <div class="col-3">Name</div>
      <div class="col">
        <span>{{ currentDevice.name }}</span>
        <q-icon dense dark size="xs" name="edit" v-if="props.allowEdit" />
        <PopupEditTextField
          v-if="props.allowEdit"
          ref="editNamePopup"
          :defaultValue="defaultDisplayedNameForPopup_"
          :initialValue="currentDevice?.userOverrides?.name"
          @update:userSetValue="notifyOfDeviceNameEdit_"
          :validator="validateDeviceName_"
          validateFailedMsg="More than 1 chars required"
          thingBeingEdited="Device Name"
        />
      </div>
    </div>
    <div class="row" v-if="aliases && aliases.length >= 1">
      <div class="col-3">{{ PluralizeNoun("Alias", aliases.length) }}</div>
      <div class="col">{{ aliases.join(", ") }}</div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col">
        <ReadOnlyTextWithHover
          :message="currentDevice.id"
          :link="
            props.includeLinkToDetailsPage ? `/#/device/${currentDevice.id}` : undefined
          "
        />
        <Link2DetailsPage
          :link="'/#/device/' + currentDevice.id"
          v-if="props.includeLinkToDetailsPage"
        />
      </div>
    </div>
    <div class="row" v-if="props.allowEdit || currentDevice?.userOverrides?.notes">
      <div class="col-3">Notes</div>
      <div class="col">
        <span>{{ currentDevice?.userOverrides?.notes }}</span>
        <q-icon dense dark size="xs" name="edit" v-if="props.allowEdit" />
        <PopupEditTextField
          ref="editNotesPopup"
          v-if="props.allowEdit"
          defaultValue=""
          :initialValue="currentDevice?.userOverrides?.notes"
          @update:userSetValue="notifyOfDeviceNotesEdit_"
          thingBeingEdited="Notes"
        />
      </div>
    </div>
    <div class="row" v-if="props.allowEdit || currentDevice?.userOverrides?.tags">
      <div class="col-3">Tags</div>
      <div class="col">
        <span>{{ currentDevice?.userOverrides?.tags?.join(", ") }}</span>
        <q-icon dense dark size="xs" name="edit" v-if="props.allowEdit" />
        <PopupEditTagsListField
          ref="editTagsPopup"
          v-if="props.allowEdit"
          :defaultValue=[]
          :initialValue="currentDevice?.userOverrides?.tags"
          @update:userSetValue="notifyOfDeviceTagsEdit_"
          thingBeingEdited="Tags"
        />
      </div>
    </div>
    <div class="row" v-if="currentDevice.type">
      <div class="col-3">
        {{ PluralizeNoun("Type", currentDevice.type.length) }}
      </div>
      <div class="col">{{ currentDevice.type.join(", ") }}</div>
    </div>
    <div class="row" v-if="currentDevice.icon">
      <div class="col-3">Icon</div>
      <div class="col">
        <img :src="currentDevice.icon.toString()" width="24" height="24" />
      </div>
    </div>
    <div class="row" v-if="currentDevice.manufacturer">
      <div class="col-3">Manufacturer</div>
      <div class="col">
        <span
          v-if="
            currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName
          "
          >{{
            currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName
          }}</span
        >
        <span v-if="currentDevice.manufacturer.webSiteURL">
          <span
            v-if="
              currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName
            "
            >;
          </span>
          Link:
          <a :href="currentDevice.manufacturer.webSiteURL" target="_blank">{{
            currentDevice.manufacturer.webSiteURL
          }}</a>
        </span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.operatingSystem">
      <div class="col-3">OS</div>
      <div class="col">
        {{ currentDevice.operatingSystem.fullVersionedName }}
      </div>
    </div>
    <div class="row" v-if="currentDevice.seen">
      <div class="col-3">Seen</div>

      <div class="col">
        <div
          class="row"
          v-for="[seenType, seenRange] in Object.entries(currentDevice.seen)"
          v-bind:key="seenType"
        >
          <div
            v-if="props.showSeenDetails || seenType == 'Ever'"
            class="col no-wrap truncateWithElipsis"
            style="min-width: 18em; max-width: 24em"
          >
            <ReadOnlyTextWithHover
              :message="FormatIDateTimeRange(seenRange) ?? ''"
              class="nowrap"
            />
          </div>
          <div v-if="props.showSeenDetails" class="col no-wrap truncateWithElipsis">
            <span v-if="seenType != 'Ever'">via</span> {{ seenType }}
          </div>
        </div>
      </div>
    </div>
    <div class="row" v-if="currentDevice.attachedNetworks && currentDeviceDetails">
      <div class="col-3">
        {{
          PluralizeNoun("Network", currentDeviceDetails.attachedFullNetworkObjects.length)
        }}
      </div>
      <div class="col">
        <div
          class="row"
          v-for="attachedNet in currentDeviceDetails.attachedFullNetworkObjects"
          v-bind:key="attachedNet.id"
        >
          <div
            class="col"
            v-if="
              props.showOldNetworks ||
              (attachedNet.seen?.upperBound &&
                DateTime.fromJSDate(attachedNet.seen?.upperBound).diffNow('minutes')
                  .minutes > -10)
            "
          >
            <div class="row">
              <div class="col no-wrap truncateWithElipsis">
                <ReadOnlyTextWithHover
                  :message="GetNetworkName(attachedNet)"
                  :popupTitle="GetNetworkName(attachedNet) + ' (' + attachedNet.id + ')'"
                  :link="GetNetworkLink(attachedNet.id)"
                  title="Network Name"
                />
              </div>
            </div>
            <div
              class="row"
              v-if="currentDevice.attachedNetworks[attachedNet.id].hardwareAddresses"
            >
              <div class="col-1" />
              <div class="col no-wrap truncateWithElipsis">
                {{
                  PluralizeNoun(
                    "Hardware Address",
                    currentDevice.attachedNetworks[attachedNet.id].hardwareAddresses
                      .length
                  )
                }}
              </div>
              <div class="col no-wrap truncateWithElipsis">
                <ReadOnlyTextWithHover
                  :message="
                    currentDevice.attachedNetworks[attachedNet.id].hardwareAddresses.join(
                      ', '
                    )
                  "
                />
              </div>
            </div>
            <div
              class="row"
              v-if="currentDevice.attachedNetworks[attachedNet.id].localAddresses"
            >
              <div class="col-1" />
              <div class="col no-wrap truncateWithElipsis">
                {{
                  PluralizeNoun(
                    "Network Address Binding",
                    currentDevice.attachedNetworks[attachedNet.id].localAddresses.length
                  )
                }}
              </div>
              <div class="col no-wrap truncateWithElipsis">
                <ReadOnlyTextWithHover
                  :message="
                    currentDevice.attachedNetworks[attachedNet.id].localAddresses.join(
                      ', '
                    )
                  "
                />
              </div>
            </div>
            <div class="row" v-if="attachedNet.seen">
              <div class="col-1" />
              <div class="col no-wrap truncateWithElipsis">Seen</div>
              <div class="col no-wrap truncateWithElipsis">
                <ReadOnlyTextWithHover
                  :message="FormatIDateTimeRange(attachedNet.seen)"
                />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3">
        {{ PluralizeNoun("Service", GetServices(currentDevice).length) }}
      </div>
      <div class="col">
        <div class="row" v-for="svc in GetServices(currentDevice)" v-bind:key="svc.name">
          <div class="col-1">
            <img
              v-if="ComputeServiceTypeIconURL(svc.name).url"
              :src="ComputeServiceTypeIconURL(svc.name).url"
              height="20"
              width="20"
            />
          </div>
          <div class="col-1">{{ svc.name }}</div>
          <div class="col">
            <div class="row wrap">
              <a
                v-for="l in svc.links"
                v-bind:href="l.href"
                v-bind:key="l.href"
                class="list-items"
                target="_blank"
                >{{ l.href }}</a
              >
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3">Open Ports</div>
      <div class="col">
        <q-btn
          class="smallBtnMargin"
          elevation="2"
          dense
          size="sm"
          @click="rescanDevice"
          v-if="!currentDevice.aggregatedBy"
          :disabled="isRescanning"
        >
          {{ isRescanning ? "**SCANNING**" : "Rescan" }}
        </q-btn>
        <span v-if="currentDevice.openPorts">{{
          currentDevice.openPorts.join(", ")
        }}</span>
      </div>
    </div>
    <div
      class="row"
      v-if="
        (currentDevice.aggregatesReversibly &&
          currentDevice.aggregatesReversibly.length) ||
        (currentDevice.aggregatesIrreversibly &&
          currentDevice.aggregatesIrreversibly.length)
      "
    >
      <div class="col-3">Aggregates</div>
      <div class="col">
        <div
          class="row wrap"
          v-if="
            currentDevice.aggregatesReversibly &&
            currentDevice.aggregatesReversibly.length
          "
        >
          <span
            v-for="aggregate in SortDeviceIDsByMostRecentFirst_(
              currentDevice.aggregatesReversibly
            )"
            v-bind:key="aggregate"
            class="aggregatesItem"
          >
            <ReadOnlyTextWithHover
              :message="GetSubDeviceDisplay_(aggregate, true)"
              :popup-title="GetSubDeviceDisplay_(aggregate, false)"
              :link="'/#/device/' + aggregate"
            />;&nbsp;
          </span>
        </div>
        <!--not supported yet, and nothing much to see here so generally won't bother listing except in details mode-->
        <div
          class="row wrap"
          v-if="
            currentDevice.aggregatesIrreversibly &&
            currentDevice.aggregatesIrreversibly.length &&
            props.showExtraDetails
          "
        >
          <span
            v-for="aggregate in SortDeviceIDsByMostRecentFirst_(
              currentDevice.aggregatesIrreversibly
            )"
            v-bind:key="aggregate"
            class="aggregatesItem"
          >
            <ReadOnlyTextWithHover
              :message="GetSubDeviceDisplay_(aggregate, true)"
              :popup-title="GetSubDeviceDisplay_(aggregate, false)"
            />;&nbsp;
          </span>
        </div>
      </div>
    </div>
    <div
      class="row"
      v-if="currentDevice.attachedNetworkInterfaces && props.showExtraDetails"
    >
      <div class="col-3">
        {{
          PluralizeNoun(
            "Attached Network Interface",
            currentDevice.attachedNetworkInterfaces.length
          )
        }}
      </div>
      <div class="col">
        <NetworkInterfacesDetails
          :network-interface-ids="currentDevice.attachedNetworkInterfaces"
          :showInactiveInterfaces="props.showInactiveInterfaces"
        />
      </div>
    </div>
    <div class="row" v-if="currentDevice.userOverrides && props.showExtraDetails">
      <div class="col-3">USEROVERRIDES</div>
      <div class="col">
        <json-viewer
          :value="currentDevice.userOverrides"
          :expand-depth="0"
          copyable
          sort
          class="debugInfoJSONViewers"
        />
      </div>
    </div>
    <div class="row" v-if="currentDevice.debugProps && props.showExtraDetails">
      <div class="col-3">DEBUG INFO</div>
      <div class="col">
        <json-viewer
          :value="currentDevice.debugProps"
          :expand-depth="0"
          copyable
          sort
          class="debugInfoJSONViewers"
        />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.list-items {
  padding-right: 1em;
}

.smallBtnMargin {
  margin-left: 1em;
  margin-right: 1em;
}

.snapshot {
  font-style: italic;
}

.aggregatesItem {
  min-width: 10em;
}
</style>
