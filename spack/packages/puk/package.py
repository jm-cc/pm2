from spack import *
import platform

class Puk(AutotoolsPackage):
    """Puk Padico micro-kernel"""
    homepage = "http://pm2.gforge.inria.fr/pm2/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/Puk')

    resource(
        name='building-tools',
        svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/building-tools',
        destination='.'
    )
    
    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')

    depends_on("pkg-config")
    depends_on("expat")
    depends_on('autoconf')

    build_directory = 'puk-build'
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--disable-trace',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimized"     % ('enable' if '+optimized' in spec else 'disable'),
        ])

        return config_args
    
    def autoreconf(self, spec, prefix):
        autogen = Executable("./autogen.sh")
        autogen()
            
        
