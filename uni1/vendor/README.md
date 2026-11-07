# Third-party source integrations

Universe 1 adapts selected open-source components and design guidance:

- **EditorAI** — https://github.com/entity12208/EditorAI
  - License: MIT; copyright notice is retained in `editorai/LICENSE`.
  - Adapted: level-design workflow and style guidance in `../design_knowledge.py`, based on upstream `src/design_knowledge.hpp`.
  - Upstream revision inspected: `3a6afd4f7a00ba07fd2ee42e94eb36e6eafb2cf6`.
- **GD-AI-bridge** — https://github.com/RibomX/GD-AI-bridge
  - License: MIT; copyright notice is retained in `gd-ai-bridge/LICENSE`.
  - Adapted: compact editor map representation and object-type prioritization in Universe's `src/main.cpp`, based on upstream `src/Api.cpp`.
  - Upstream revision inspected: `b59f0c09def024729b55a91db300980b00696c2a`.
- **LayoutGenerator** — https://github.com/ProfessorDragon/LayoutGenerator
  - Its repository snapshot did not expose a license file. No source code from it is copied into Universe.

The MIT license texts are included beside this notice. Adaptations are not official releases
of, or endorsed by, the upstream projects. Universe's target remains Geometry Dash 2.2091;
upstream Geode mods targeting 2.2081 are not imported as binary plugins.
