# Heroicons Reference Guide

## How to Use Heroicons in Documentation

Heroicons are embedded as inline SVG HTML. Copy and paste any of the examples below into your markdown files.

## Common Icons Used in SmartFall Docs

### Hardware & Components
```html
<!-- Microcontroller -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M9 2a1 1 0 000 2h2V1H9zM7 8a2 2 0 11-4 0 2 2 0 014 0zM7 20a2 2 0 11-4 0 2 2 0 014 0zM9 9a1 1 0 100-2H5.236A2.002 2.002 0 003 9c0 .659.214 1.268.594 1.776.356.488.89.723 1.5.723H9zM9 20a1 1 0 100-2h-3.464a2.5 2.5 0 010-5H9a1 1 0 100-2H5.564a4.5 4.5 0 010 9H9zM21 1a1 1 0 00-1 1v2h-2a1 1 0 100 2h2v2a1 1 0 102 0V4a1 1 0 00-1-1zM16.5 9a1.5 1.5 0 11-3 0 1.5 1.5 0 013 0zM21 20a1 1 0 01-1-1v-2h-2a1 1 0 110-2h2v-2a1 1 0 112 0v2h2a1 1 0 110 2h-2v2a1 1 0 01-1 1z"/></svg>

<!-- Sensor -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm0-14c-3.31 0-6 2.69-6 6s2.69 6 6 6 6-2.69 6-6-2.69-6-6-6z"/></svg>

<!-- Battery -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M15 4h-2V2h-2v2H9c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h6c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H9V6h6v12z"/></svg>

<!-- Wifi -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z"/></svg>

<!-- Bluetooth -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M17.71 7.71L12 2h-1v7.59L6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 11 14.41V22h1l5.71-5.71-4.3-4.29 4.3-4.29zM13 5.83l1.88 1.88L13 9.59V5.83zm1.88 10.46L13 18.17v-3.76l1.88 1.88z"/></svg>

<!-- Alert/Warning -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/></svg>

<!-- Check/Success -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>

<!-- Settings/Configuration -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.62l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.48.1.62l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.62l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.48-.1-.62l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>

<!-- Power/Energy -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M16.6 10.88c.26-.9.4-1.82.4-2.88 0-6.63-5.37-12-12-12S-7.4 1.37-7.4 8c0 1.06.14 1.98.4 2.88C-8.99 11.55-10 13.47-10 15.5c0 5.24 4.26 9.5 9.5 9.5s9.5-4.26 9.5-9.5c0-2.03-1.01-3.95-2.4-5.12zM0 9.5C0 4.26 4.26 0 9.5 0S19 4.26 19 9.5c0 .62-.05 1.23-.15 1.82.9.64 1.76 1.38 2.52 2.2.14-1.02.23-2.05.23-3.12 0-7.18-5.82-13-13-13S-4 1.32-4 8.5c0 1.07.09 2.1.23 3.12.76-.82 1.62-1.56 2.52-2.2-.1-.59-.15-1.2-.15-1.82z"/></svg>

<!-- Graph/Chart -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M5 9.2h3V19H5zM10.6 5h2.8v14h-2.8zm5.6 8H19v6h-2.8z"/></svg>

<!-- Sound/Audio -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>

<!-- Mobile/Phone -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M17 2H7c-1.1 0-1.99.9-1.99 2v16c0 1.1.89 2 1.99 2h10c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm-5 18c-.83 0-1.5-.67-1.5-1.5s.67-1.5 1.5-1.5 1.5.67 1.5 1.5-.67 1.5-1.5 1.5zm5-3H7V4h10v13z"/></svg>

<!-- Cloud/Server -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M19.35 10.04C18.67 6.59 15.64 4 12 4c-1.48 0-2.85.43-4.01 1.17l1.46 1.46C10.21 5.23 11.08 5 12 5c3.04 0 5.5 2.46 5.5 5.5v.5H19c1.66 0 3 1.34 3 3 0 1.13-.64 2.11-1.56 2.62l1.45 1.45C23.16 15.63 24 14.08 24 12.5c0-2.64-2.05-4.78-4.65-4.96z"/></svg>

<!-- Code/Terminal -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M4 6h16v2H4zm0 5h16v2H4zm0 5h16v2H4z"/></svg>

<!-- Document/File -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-8-6z"/></svg>

<!-- Book/Documentation -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M4 6h16v2H4zm0 5h16v2H4zm0 5h16v2H4z"/></svg>

<!-- Flame/Fire (for alerts) -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M13.5.67s.74 2.65.74 4.8c0 2.06-1.35 3.73-3.41 3.73-2.07 0-3.63-1.67-3.63-3.73l.03-.36C5.21 7.51 4 10.3 4 13.46 4 19.22 8.7 24 15.5 24c6.8 0 11.5-4.78 11.5-10.54C27 15.7 26.92 14.9 26.54 13.9"/></svg>

<!-- Heart -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>

<!-- Zap/Flash (for emergency) -->
<svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>
```

## Usage Example

In any markdown file, you can use these SVGs directly:

```markdown
## <svg class="heroicon" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm0-14c-3.31 0-6 2.69-6 6s2.69 6 6 6 6-2.69 6-6-2.69-6-6-6z"/></svg> System Overview

Content here...
```

## Icon List

The icons above represent these concepts:

| Icon | Usage | File |
|------|-------|------|
| Microcontroller | ESP32 references | firmware, hardware |
| Sensor | Sensor data, measurements | algorithm, firmware |
| Battery | Power, battery life | hardware/power |
| WiFi | Network connectivity | communication, api |
| Bluetooth | BLE, mobile alerts | communication, api |
| Alert | Warnings, emergencies | algorithm, troubleshooting |
| Check | Success, verified | testing, features |
| Settings | Configuration, options | configuration |
| Power | Energy, power consumption | hardware/power |
| Graph | Data visualization, charts | algorithm, testing |
| Sound | Audio alerts | firmware/audio |
| Mobile | Mobile app, BLE app | communication, api |
| Cloud | Server, WiFi transmission | communication, api |
| Code | Code examples, snippets | firmware, api |
| Document | Files, references | all |
| Book | Documentation | general |
| Flame | High priority, fire/danger | troubleshooting |
| Heart | Heart rate sensor | firmware/sensors |
| Zap | Emergency, immediate action | algorithm, troubleshooting |

## Colors with Heroicons

Heroicons use `currentColor`, so they inherit the text color from their context:

```markdown
- **Red Icon**: <span style="color: #dc2626;">Red text with icon SVG</span>
- **Orange Icon**: <span style="color: #ea580c;">Orange text with icon SVG</span>
- **Blue Icon**: <span style="color: #2563eb;">Blue text with icon SVG</span>
```

## Integration Notes

- All icons scale with text size due to `em` units
- SVG paths use `fill="currentColor"` for automatic theme color support
- Custom CSS in `custom.css` handles sizing in different contexts (headings, tables, etc.)
- Heroicons work in light and dark modes automatically
