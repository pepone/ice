set -xeuo pipefail

case "$CHANNEL" in
  "3.8")
    REPO_ID=ossrh
    SOURCE_URL="https://ossrh-staging-api.central.sonatype.com/service/local/staging/deploy/maven2/"
    ;;
  "nightly")
    REPO_ID=maven-nightly
    SOURCE_URL="https://download.zeroc.com/nexus/repository/maven-nightly/"
    ;;
  *)
    echo "Unsupported channel: $CHANNEL"
    exit 1
    ;;
esac

# Generate Maven settings.xml
mkdir -p ~/.m2
cat > ~/.m2/settings.xml <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0 https://maven.apache.org/xsd/settings-1.0.0.xsd">
  <servers>
    <server>
      <id>gradlePluginPortal</id>
      <username>${GRADLE_PUBLISH_KEY}</username>
      <password>${GRADLE_PUBLISH_SECRET}</password>
    </server>
  </servers>
</settings>
EOF

# Import the signing GPG key.
#echo "$GPG_KEY" | gpg --batch --import

# Copy the JAR and POM files
mkdir -p lib
cp -pf "${STAGING_DIR}"/java-packages/*.jar lib
cp -pf "${STAGING_DIR}"/java-packages/*.pom lib

ice_version=$(basename "$(ls lib/ice-*-sources.jar)" | sed -E 's/^ice-(.+)-sources\.jar$/\1/')
components=(ice glacier2 icebt icebox icediscovery icelocatordiscovery icegrid icestorm)

plugin_staging_dir="${STAGING_DIR}/slice-tools-packages"

if [ "$CHANNEL" = "nightly" ]; then
  echo "Publishing Slice Tools plugin to ZeroC Nexus"
  PLUGIN_REPO_URL="${SOURCE_URL}"
  PLUGIN_REPO_ID="${REPO_ID}"
  
  mkdir -p plugin
  
  cp "${plugin_staging_dir}/com/zeroc/slice-tools/${ice_version}/"*.jar "plugin/slice-tools-${ice_version}.jar"
  cp "${plugin_staging_dir}/com/zeroc/slice-tools/${ice_version}/"*.pom "plugin/slice-tools-${ice_version}.pom"

  plugin_jar="plugin/slice-tools-${ice_version}.jar"
  plugin_pom="plugin/slice-tools-${ice_version}.pom"

  mvn org.apache.maven.plugins:maven-gpg-plugin:3.2.4:sign-and-deploy-file \
    -Dgpg.keyname="${GPG_KEY_ID}" \
    -Dfile="${plugin_jar}" \
    -DpomFile="${plugin_pom}" \
    -Durl="${PLUGIN_REPO_URL}" \
    -DrepositoryId="${PLUGIN_REPO_ID}" || { echo "Failed to publish plugin"; exit 1; }

  cp "${plugin_staging_dir}/com/zeroc/slice-tools/com.zeroc.slice-tools.gradle.plugin/${ice_version}/"*.pom \
    "plugin/com.zeroc.slice-tools.gradle.plugin-${ice_version}.pom"

  plugin_marker_pom="plugin/com.zeroc.slice-tools.gradle.plugin-${ice_version}.pom"

  echo "Publishing plugin marker POM"

  mvn org.apache.maven.plugins:maven-gpg-plugin:3.2.4:sign-and-deploy-file \
    -Dgpg.keyname="${GPG_KEY_ID}" \
    -Dfile="${plugin_marker_pom}" \
    -Dpackaging=pom \
    -DgroupId="com.zeroc.slice-tools" \
    -DartifactId="com.zeroc.slice-tools.gradle.plugin" \
    -Dversion="${ice_version}" \
    -Durl="${PLUGIN_REPO_URL}" \
    -DrepositoryId="${PLUGIN_REPO_ID}" || { echo "Failed to publish plugin marker"; exit 1; }
else
  echo "Publishing Slice Tools plugin to Gradle Plugin Portal"
  
  # Create a temporary Gradle project to publish the pre-built artifacts
  temp_dir=$(mktemp -d)
  trap "rm -rf ${temp_dir}" EXIT
  
  # Copy the pre-built plugin artifacts to a local Maven repository structure
  local_repo="${temp_dir}/local-repo"
  mkdir -p "${local_repo}"
  cp -r "${plugin_staging_dir}/"* "${local_repo}/"
  
  # Create a minimal Gradle build script to publish from the local repo
  cat > "${temp_dir}/build.gradle" <<'EOF'
plugins {
    id 'maven-publish'
    id 'com.gradle.plugin-publish' version '1.3.0'
}

group = 'com.zeroc'
version = project.findProperty('pluginVersion') ?: '0.0.0'

repositories {
    maven {
        url = uri("${project.projectDir}/local-repo")
    }
}

gradlePlugin {
    website = 'https://github.com/zeroc-ice/ice'
    vcsUrl = 'https://github.com/zeroc-ice/ice.git'
    plugins {
        sliceTools {
            id = 'com.zeroc.slice-tools'
            displayName = 'ZeroC Slice Tools Plugin'
            description = 'Gradle plugin for compiling Slice files to Java using slice2java'
            tags.set(['zeroc', 'ice', 'slice', 'code-generation'])
            implementationClass = 'com.zeroc.gradle.plugins.slice.SlicePlugin'
        }
    }
}

// Configure publishing to use pre-built artifacts
publishing {
    publications {
        pluginMaven(MavenPublication) {
            artifact(file("local-repo/com/zeroc/slice-tools/${version}/slice-tools-${version}.jar"))
            artifact(file("local-repo/com/zeroc/slice-tools/${version}/slice-tools-${version}-sources.jar")) {
                classifier = 'sources'
            }
            artifact(file("local-repo/com/zeroc/slice-tools/${version}/slice-tools-${version}-javadoc.jar")) {
                classifier = 'javadoc'
            }
            pom {
                name = 'ZeroC Slice Tools'
                description = 'Slice compiler tools for ZeroC Ice'
            }
        }
    }
}
EOF
  
  cat > "${temp_dir}/settings.gradle" <<EOF
rootProject.name = 'slice-tools'
EOF
  
  # Copy pre-built artifacts to temp directory for Gradle to find
  cp -r "${plugin_staging_dir}" "${temp_dir}/local-repo"
  
  # Publish using Gradle
  cd "${temp_dir}"
  gradle publishPlugins \
    -PpluginVersion="${ice_version}" \
    -Pgradle.publish.key="${GRADLE_PUBLISH_KEY}" \
    -Pgradle.publish.secret="${GRADLE_PUBLISH_SECRET}" \
    || { echo "Failed to publish plugin to Gradle Plugin Portal"; exit 1; }
  
  echo "Successfully published slice-tools plugin version ${ice_version} to Gradle Plugin Portal"
fi
