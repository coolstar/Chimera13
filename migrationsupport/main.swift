import Foundation

dlopen("/usr/lib/pspawn_payload-stg2.dylib", RTLD_NOW)

func runCmd(cmd: String) -> Int32 {
    let args = ["/bin/sh", "-c", cmd]
    let argv: [UnsafeMutablePointer<CChar>?] = args.map { $0.withCString(strdup) }
    defer { for case let arg? in argv { free(arg) } }
    
    var pid = pid_t(0)
    var status = posix_spawn(&pid, "/bin/sh", nil, nil, argv + [nil], environ)
    if status == 0 {
        if waitpid(pid, &status, 0) == -1 {
            perror("waitpid")
        }
    } else {
        print("posix_spawn:", status)
    }
    return status
}

func migrate() -> Bool {
    if FileManager.default.fileExists(atPath: "/.procursus_strapped") {
        return true
    }
    
    let version = (kCFCoreFoundationVersionNumber / 100).rounded(.down) * 100
    let versionString = String(format: "%.0f", version)
    
    let preferencesFile = "/tmp/migrate.prefs"
    let sourcesFile = "/tmp/migrate.sources"
    
    let migrationPrefs = """
        Package: *
        Pin: release o=Procursus
        Pin-Priority: 1001

        Package: *
        Pin: release o="Odyssey Repo"
        Pin-Priority: 1001
        """
    
    let migrationSources = """
        Types: deb
        URIs: https://apt.procurs.us/
        Suites: iphoneos-arm64/\(versionString)
        Components: main

        Types: deb
        URIs: https://repo.theodyssey.dev/
        Suites: ./
        Components:
        """
    
    guard (try? migrationPrefs.write(toFile: preferencesFile, atomically: false, encoding: .utf8)) != nil else {
        return false
    }
    guard (try? migrationSources.write(toFile: sourcesFile, atomically: false, encoding: .utf8)) != nil else {
        return false
    }
    
    let aptArgs = ["-y",
                   "-oDir::Etc=",
                   "-oDir::Etc::sourcelist=\(sourcesFile)",
                   "-oDir::Etc::preferences=\(preferencesFile)",
                   "-oAPT::Get::AllowUnauthenticated=true",
                   "-oAPT::Force-LoopBreak=true",
                   "-oAcquire::AllowDowngradeToInsecureRepositories=true",
                   "-oAcquire::AllowInsecureRepositories=true",
                   "--allow-remove-essential",
                   "--allow-downgrades"]
    let aptGet = "/usr/bin/apt-get " + aptArgs.joined(separator: " ")

    let leftoverEach = ["\(preferencesFile) \(sourcesFile)",
                        "/.installed_unc0ver /.bootstrapped /jb /private/var/jb",
                        "/etc/apt/preferences.d/cydia /etc/apt/preferences.d/checkra1n",
                        "/etc/apt/sources.list.d/mobilesubstrate.list coreutils*.deb"]
    let leftovers = leftoverEach.joined(separator: " ")
    
    guard runCmd(cmd: "\(aptGet) update") == 0,
    runCmd(cmd: "/usr/bin/dpkg --force-all --remove jailbreak-resources diskdev-cmds org.coolstar.sileo vim apt7 apt7-lib apt7-key libplist libplist++3 libplist++-dev") == 0 else {
        return false
    }
    
    if !FileManager.default.fileExists(atPath: "/bin/launchctl") {
        var attributes = [FileAttributeKey: Any]()
        attributes[.posixPermissions] = 0o755
        FileManager.default.createFile(atPath: "/bin/launchctl", contents: nil, attributes: attributes)
    }
    guard runCmd(cmd: "\(aptGet) install libncursesw6") == 0 else {
        return false
    }
    if !FileManager.default.fileExists(atPath: "/usr/lib/libncurses.6.dylib") {
        do {
            try FileManager.default.createSymbolicLink(atPath: "/usr/lib/libncurses.6.dylib", withDestinationPath: "/usr/lib/libncursesw.6.dylib")
        } catch {
            return false
        }
    }
    guard runCmd(cmd: "\(aptGet) install readline bash xz-utils") == 0,
    runCmd(cmd: "\(aptGet) dist-upgrade") == 0,
    runCmd(cmd: "\(aptGet) download coreutils") == 0,
    runCmd(cmd: "/usr/bin/dpkg --force-all -i coreutils*.deb") == 0,
    runCmd(cmd: "/usr/bin/dpkg --force-all --remove cydia cydia-lproj org.thebigboss.repo.icons") == 0,
    runCmd(cmd: "\(aptGet) install -f --force-yes") == 0,
    runCmd(cmd: "\(aptGet) install org.coolstar.libhooker cydia") == 0,
    runCmd(cmd: "/usr/bin/dpkg --force-all --remove apt1.4 trustinjector jbctl xz lzma libapt-pkg5.0 libapt pcre pcre2 gcrypt com.saurik.substrate.safemode science.xnu.substituted") == 0,
    runCmd(cmd: "uicache -p /Applications/SafeMode.app") == 0,
    runCmd(cmd: "/usr/bin/rm -rf \(leftovers)") == 0 else {
        return false
    }
    
    let tweakInjectPath = "/usr/lib/TweakInject"
    let substratePath = "/Library/MobileSubstrate/DynamicLibraries"
    if ((try? FileManager.default.destinationOfSymbolicLink(atPath: substratePath)) == nil) &&
        FileManager.default.fileExists(atPath: substratePath) {
        if !FileManager.default.fileExists(atPath: tweakInjectPath) {
            guard (try? FileManager.default.createDirectory(atPath: tweakInjectPath, withIntermediateDirectories: false, attributes: nil)) != nil else {
                return false
            }
        }
        if let tweaks = try? FileManager.default.contentsOfDirectory(atPath: substratePath) {
            for tweak in tweaks {
                try? FileManager.default.moveItem(atPath: "\(substratePath)/\(tweak)", toPath: "\(tweakInjectPath)/\(tweak)")
            }
        }
        guard (try? FileManager.default.removeItem(atPath: substratePath)) != nil,
            (try? FileManager.default.createSymbolicLink(atPath: substratePath, withDestinationPath: tweakInjectPath)) != nil else {
            return false
        }
    }
    
    let procursusPrefs = """
        Package: *
        Pin: release o=Procursus
        Pin-Priority: 1001
        """
    
    let procursusSources = """
        Types: deb
        URIs: https://apt.procurs.us/
        Suites: iphoneos-arm64/\(versionString)
        Components: main
        """
    
    guard (try? procursusPrefs.write(toFile: "/private/etc/apt/preferences.d/procursus", atomically: false, encoding: .utf8)) != nil,
        (try? procursusSources.write(toFile: "/private/etc/apt/sources.list.d/procursus.sources", atomically: false, encoding: .utf8)) != nil else {
        return false
    }
    
    guard (try? "".write(toFile: "/.procursus_strapped", atomically: false, encoding: .utf8)) != nil else {
        return false
    }
    return true
}

guard migrate() else {
    exit(1)
}
